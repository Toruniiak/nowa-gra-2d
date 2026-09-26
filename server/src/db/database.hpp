#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "auth/password.hpp"

struct sqlite3;

namespace game::db {

using AccountId = int64_t;
using CharacterId = int64_t;

struct Account {
  AccountId id;
  std::string username;
  auth::PasswordHash passwordHash;
};

struct CharacterSummary {
  CharacterId id;
  std::string name;
};

struct CharacterRecord {
  CharacterId id;
  AccountId accountId;
  std::string name;
  float x;
  float y;
};

// SQLite-backed persistence for accounts and characters. Chosen over a
// client-server DB (MySQL/MariaDB/Postgres) for this phase: embedded, no
// separate service to run/secure/backup during early development, and this
// project's data volume (accounts + character positions) has no need for a
// separate DB process yet. Revisit only if/when a real concurrency or
// scaling need appears (see docs/ARCHITECTURE.md) — not preemptively.
//
// Not thread-safe by design: the server is single-threaded (see
// net/server.hpp), so no internal locking is implemented. Adding threads
// later must revisit this.
class Database {
 public:
  // Opens (creating if absent) the SQLite file at `path` and ensures the
  // schema exists. Throws std::runtime_error on failure — a DB that can't
  // be opened/migrated is a startup-fatal condition, not something to run
  // degraded against.
  explicit Database(const std::string& path);
  ~Database();

  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  // Returns nullopt if the username is already taken (case-sensitive) or on
  // internal error. Never overwrites an existing account.
  std::optional<AccountId> createAccount(const std::string& username,
                                          const auth::PasswordHash& passwordHash);

  // Returns nullopt if no account with this username exists.
  std::optional<Account> findAccountByUsername(const std::string& username);

  std::vector<CharacterSummary> listCharacters(AccountId accountId);

  // Returns nullopt if the name is already taken (character names are
  // globally unique, matching typical MMO conventions) or on internal
  // error. Spawns at (0, 0) — see docs/GAME_DESIGN.md for future starting
  // areas.
  std::optional<CharacterId> createCharacter(AccountId accountId, const std::string& name);

  // Returns nullopt if the character doesn't exist or does not belong to
  // accountId. Callers must check ownership this way rather than trusting a
  // client-provided character id on its own — see docs/ARCHITECTURE.md.
  std::optional<CharacterRecord> getOwnedCharacter(AccountId accountId, CharacterId characterId);

  void saveCharacterPosition(CharacterId characterId, float x, float y);

 private:
  sqlite3* db_ = nullptr;

  void execOrThrow(const std::string& sql);
};

}  // namespace game::db
