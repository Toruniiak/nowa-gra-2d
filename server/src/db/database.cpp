#include "db/database.hpp"

#include <sqlite3.h>

#include <chrono>
#include <stdexcept>

namespace game::db {

namespace {

// RAII wrapper around sqlite3_stmt so early returns (a common pattern below,
// e.g. "row not found") can't leak a prepared statement.
class Stmt {
 public:
  Stmt(sqlite3* db, const std::string& sql) {
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
      throw std::runtime_error("sqlite3_prepare_v2 failed: " + std::string(sqlite3_errmsg(db)));
    }
  }
  ~Stmt() { sqlite3_finalize(stmt_); }

  Stmt(const Stmt&) = delete;
  Stmt& operator=(const Stmt&) = delete;

  sqlite3_stmt* get() const { return stmt_; }

 private:
  sqlite3_stmt* stmt_ = nullptr;
};

int64_t nowUnix() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

}  // namespace

Database::Database(const std::string& path) {
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    const std::string err = db_ ? sqlite3_errmsg(db_) : "unknown error";
    if (db_) sqlite3_close(db_);
    throw std::runtime_error("Failed to open database '" + path + "': " + err);
  }

  execOrThrow("PRAGMA foreign_keys = ON;");
  execOrThrow(
      "CREATE TABLE IF NOT EXISTS accounts ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  username TEXT NOT NULL UNIQUE,"
      "  password_hash BLOB NOT NULL,"
      "  password_salt BLOB NOT NULL,"
      "  created_at INTEGER NOT NULL"
      ");");
  execOrThrow(
      "CREATE TABLE IF NOT EXISTS characters ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  account_id INTEGER NOT NULL REFERENCES accounts(id),"
      "  name TEXT NOT NULL UNIQUE,"
      "  pos_x REAL NOT NULL DEFAULT 0,"
      "  pos_y REAL NOT NULL DEFAULT 0,"
      "  created_at INTEGER NOT NULL"
      ");");
  execOrThrow("CREATE INDEX IF NOT EXISTS idx_characters_account ON characters(account_id);");
}

Database::~Database() {
  if (db_) sqlite3_close(db_);
}

void Database::execOrThrow(const std::string& sql) {
  char* errMsg = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::string err = errMsg ? errMsg : "unknown error";
    sqlite3_free(errMsg);
    throw std::runtime_error("sqlite3_exec failed: " + err);
  }
}

std::optional<AccountId> Database::createAccount(const std::string& username,
                                                   const auth::PasswordHash& passwordHash) {
  Stmt stmt(db_,
            "INSERT INTO accounts (username, password_hash, password_salt, created_at) "
            "VALUES (?, ?, ?, ?);");
  sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_blob(stmt.get(), 2, passwordHash.hash.data(),
                     static_cast<int>(passwordHash.hash.size()), SQLITE_TRANSIENT);
  sqlite3_bind_blob(stmt.get(), 3, passwordHash.salt.data(),
                     static_cast<int>(passwordHash.salt.size()), SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 4, nowUnix());

  const int rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    // Most commonly SQLITE_CONSTRAINT (username already exists).
    return std::nullopt;
  }
  return sqlite3_last_insert_rowid(db_);
}

std::optional<Account> Database::findAccountByUsername(const std::string& username) {
  Stmt stmt(db_,
            "SELECT id, username, password_hash, password_salt FROM accounts "
            "WHERE username = ?;");
  sqlite3_bind_text(stmt.get(), 1, username.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    return std::nullopt;
  }

  Account account;
  account.id = sqlite3_column_int64(stmt.get(), 0);
  account.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));

  const auto* hashBlob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt.get(), 2));
  const int hashLen = sqlite3_column_bytes(stmt.get(), 2);
  account.passwordHash.hash.assign(hashBlob, hashBlob + hashLen);

  const auto* saltBlob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt.get(), 3));
  const int saltLen = sqlite3_column_bytes(stmt.get(), 3);
  account.passwordHash.salt.assign(saltBlob, saltBlob + saltLen);

  return account;
}

std::vector<CharacterSummary> Database::listCharacters(AccountId accountId) {
  Stmt stmt(db_, "SELECT id, name FROM characters WHERE account_id = ? ORDER BY id;");
  sqlite3_bind_int64(stmt.get(), 1, accountId);

  std::vector<CharacterSummary> result;
  while (sqlite3_step(stmt.get()) == SQLITE_ROW) {
    CharacterSummary summary;
    summary.id = sqlite3_column_int64(stmt.get(), 0);
    summary.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 1));
    result.push_back(std::move(summary));
  }
  return result;
}

std::optional<CharacterId> Database::createCharacter(AccountId accountId, const std::string& name) {
  Stmt stmt(db_,
            "INSERT INTO characters (account_id, name, pos_x, pos_y, created_at) "
            "VALUES (?, ?, 0, 0, ?);");
  sqlite3_bind_int64(stmt.get(), 1, accountId);
  sqlite3_bind_text(stmt.get(), 2, name.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 3, nowUnix());

  const int rc = sqlite3_step(stmt.get());
  if (rc != SQLITE_DONE) {
    return std::nullopt;
  }
  return sqlite3_last_insert_rowid(db_);
}

std::optional<CharacterRecord> Database::getOwnedCharacter(AccountId accountId,
                                                             CharacterId characterId) {
  Stmt stmt(db_,
            "SELECT id, account_id, name, pos_x, pos_y FROM characters "
            "WHERE id = ? AND account_id = ?;");
  sqlite3_bind_int64(stmt.get(), 1, characterId);
  sqlite3_bind_int64(stmt.get(), 2, accountId);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    return std::nullopt;
  }

  CharacterRecord record;
  record.id = sqlite3_column_int64(stmt.get(), 0);
  record.accountId = sqlite3_column_int64(stmt.get(), 1);
  record.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt.get(), 2));
  record.x = static_cast<float>(sqlite3_column_double(stmt.get(), 3));
  record.y = static_cast<float>(sqlite3_column_double(stmt.get(), 4));
  return record;
}

void Database::saveCharacterPosition(CharacterId characterId, float x, float y) {
  Stmt stmt(db_, "UPDATE characters SET pos_x = ?, pos_y = ? WHERE id = ?;");
  sqlite3_bind_double(stmt.get(), 1, x);
  sqlite3_bind_double(stmt.get(), 2, y);
  sqlite3_bind_int64(stmt.get(), 3, characterId);
  sqlite3_step(stmt.get());
}

}  // namespace game::db
