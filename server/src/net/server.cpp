#include "net/server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/err.h>

#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>

#include "auth/password.hpp"

namespace game {

namespace {

// Per-connection auth throttling (see Client::lastAuthAttempt).
constexpr auto kAuthAttemptInterval = std::chrono::seconds(1);
constexpr int kMaxFailedAuthAttempts = 5;

// Memory bounds per connection: a client sending bytes without a newline,
// or never reading what we send, must not grow server memory unboundedly.
constexpr std::size_t kMaxInboundLineBytes = 1024;
constexpr std::size_t kMaxOutboundBufferBytes = 64 * 1024;

// A connection must finish the TLS handshake and log in within this time,
// or it's dropped — otherwise idle/half-open connections (slowloris) could
// hold file descriptors forever.
constexpr auto kLoginDeadline = std::chrono::seconds(60);

// Shared input validation for account/character names. Kept deliberately
// conservative (ASCII alnum + a few separators) — see docs/NETWORKING.md;
// this is not yet a security boundary against anything beyond obviously
// malformed input (SQL injection is already ruled out by using bound
// parameters in db::Database, not by this validation).
bool isValidUsername(const std::string& s) {
  if (s.size() < 3 || s.size() > 32) return false;
  for (unsigned char c : s) {
    if (!std::isalnum(c) && c != '_') return false;
  }
  return true;
}

bool isValidPassword(const std::string& s) { return s.size() >= 6 && s.size() <= 128; }

bool isValidCharacterName(const std::string& s) {
  if (s.size() < 3 || s.size() > 20) return false;
  if (s.front() == ' ' || s.back() == ' ') return false;
  for (unsigned char c : s) {
    if (!std::isalnum(c) && c != ' ') return false;
  }
  return true;
}

// Reads the remainder of the line (trimming exactly one leading separator
// space), so passwords and character names may contain spaces without
// breaking on `istringstream::operator>>` word-splitting.
std::string readRestOfLine(std::istringstream& iss) {
  std::string rest;
  std::getline(iss, rest);
  if (!rest.empty() && rest.front() == ' ') rest.erase(0, 1);
  return rest;
}

}  // namespace

TcpServer::TcpServer(uint16_t port, net::TlsContext& tls, db::Database& database)
    : port_(port), tls_(tls), db_(database) {}

TcpServer::~TcpServer() {
  for (auto& c : clients_) {
    if (c.ssl) {
      SSL_shutdown(c.ssl);
      SSL_free(c.ssl);
    }
    if (c.fd >= 0) ::close(c.fd);
  }
  if (listenFd_ >= 0) ::close(listenFd_);
}

bool TcpServer::start() {
  listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (listenFd_ < 0) {
    std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
    return false;
  }

  int opt = 1;
  ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "bind() on port " << port_ << " failed: " << std::strerror(errno) << "\n";
    return false;
  }
  if (::listen(listenFd_, 16) < 0) {
    std::cerr << "listen() failed: " << std::strerror(errno) << "\n";
    return false;
  }
  return true;
}

void TcpServer::runLoop(World& world, int tickIntervalMs, const volatile std::sig_atomic_t& stop) {
  std::cout << "Server listening on port " << port_ << " (TLS, tick=" << tickIntervalMs
            << "ms)" << std::endl;
  while (!stop) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(listenFd_, &readSet);
    int maxFd = listenFd_;
    for (auto& c : clients_) {
      FD_SET(c.fd, &readSet);
      if (c.fd > maxFd) maxFd = c.fd;
    }

    timeval timeout{};
    timeout.tv_sec = tickIntervalMs / 1000;
    timeout.tv_usec = (tickIntervalMs % 1000) * 1000;

    const int ready = ::select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
    if (ready < 0) {
      if (errno == EINTR) continue;
      std::cerr << "select() failed: " << std::strerror(errno) << "\n";
      break;
    }

    if (ready > 0 && FD_ISSET(listenFd_, &readSet)) {
      acceptNew();
    }

    // Called every tick regardless of `ready`: a stalled TLS handshake or a
    // buffered outbound write that returned WANT_WRITE must still be
    // retried on the next tick timeout, not only when the read-select
    // happens to report the fd ready again. See docs/NETWORKING.md.
    readClients(world);

    broadcastDirty(world);
  }

  // Graceful shutdown (SIGINT/SIGTERM): positions are otherwise saved only
  // when a player disconnects, so a plain server restart would silently
  // lose every online player's progress since login.
  for (const auto& c : clients_) {
    saveCharacter(c, world);
  }
  std::cout << "Shutdown: saved " << clients_.size() << " connection(s)" << std::endl;
}

void TcpServer::saveCharacter(const Client& client, World& world) {
  if (client.entityId == 0 || !client.characterId) return;
  auto it = world.entities().find(client.entityId);
  if (it != world.entities().end()) {
    db_.saveCharacterPosition(*client.characterId, it->second.x, it->second.y);
  }
}

void TcpServer::acceptNew() {
  sockaddr_in clientAddr{};
  socklen_t len = sizeof(clientAddr);
  const int fd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
  if (fd < 0) return;
  if (fd >= FD_SETSIZE) {
    // select()'s fd_set can't represent it — FD_SET on it would write out of
    // bounds. Refuse the connection rather than corrupt memory. (Moving off
    // select() to poll/epoll is the real fix once player counts need it.)
    ::close(fd);
    return;
  }

  // Non-blocking: required so a slow/stalled TLS handshake or partial
  // read/write can never block the single-threaded select() loop for every
  // other client.
  const int flags = ::fcntl(fd, F_GETFL, 0);
  ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  Client client;
  client.fd = fd;
  client.ssl = SSL_new(tls_.raw());
  if (!client.ssl) {
    ::close(fd);
    return;
  }
  SSL_set_fd(client.ssl, fd);
  clients_.push_back(std::move(client));

  // Kick off the handshake immediately; if it doesn't complete in one
  // non-blocking pass (the common case), serviceHandshake() retries it on
  // subsequent ticks.
  serviceHandshake(clients_.back());

  std::cout << "Client connected (TLS handshake in progress), fd " << fd << "\n";
}

void TcpServer::serviceHandshake(Client& client) {
  // Required before every SSL I/O call whose result feeds SSL_get_error():
  // OpenSSL's per-thread error queue is global, not per-SSL-object. A
  // leftover entry from an *unrelated* connection's earlier real error
  // makes SSL_get_error() misreport this call's ordinary WANT_READ/
  // WANT_WRITE as SSL_ERROR_SSL — a real bug found via testing that
  // cascaded into disconnecting unrelated, healthy clients (see
  // docs/CHANGELOG.md).
  ERR_clear_error();
  const int rc = SSL_accept(client.ssl);
  if (rc == 1) {
    client.handshakeDone = true;
    return;
  }
  const int err = SSL_get_error(client.ssl, rc);
  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    return;  // retry next tick
  }
  client.fatal = true;
}

void TcpServer::readClients(World& world) {
  for (std::size_t i = 0; i < clients_.size();) {
    Client& c = clients_[i];

    if (!c.accountId && std::chrono::steady_clock::now() - c.connectedAt > kLoginDeadline) {
      c.fatal = true;
    }
    if (!c.handshakeDone && !c.fatal) {
      serviceHandshake(c);
    }
    if (c.handshakeDone && !c.fatal) {
      serviceRead(c, world);
    }
    if (!c.fatal) {
      flushOutbuf(c);
    }

    if (c.fatal) {
      removeClient(i, world);
      continue;  // do not advance i: a client shifted into this slot
    }
    ++i;
  }
}

void TcpServer::serviceRead(Client& c, World& world) {
  char buf[512];
  for (;;) {
    ERR_clear_error();  // see serviceHandshake() for why this is required
    const int n = SSL_read(c.ssl, buf, sizeof(buf));
    if (n > 0) {
      c.inbuf.append(buf, static_cast<std::size_t>(n));
      continue;  // drain any further TLS records already buffered internally
    }
    const int err = SSL_get_error(c.ssl, n);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      break;  // no more data available right now
    }
    c.fatal = true;  // SSL_ERROR_ZERO_RETURN (clean close) or a real error
    break;
  }

  std::size_t pos;
  while (!c.fatal && (pos = c.inbuf.find('\n')) != std::string::npos) {
    std::string line = c.inbuf.substr(0, pos);
    c.inbuf.erase(0, pos + 1);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    handleLine(c, line, world);
  }
  if (c.inbuf.size() > kMaxInboundLineBytes) {
    c.fatal = true;  // no protocol line is anywhere near this long
  }
}

void TcpServer::handleLine(Client& client, const std::string& line, World& world) {
  std::istringstream iss(line);
  std::string cmd;
  iss >> cmd;

  if (cmd == "REGISTER") {
    handleRegister(client, iss);
  } else if (cmd == "LOGIN") {
    handleLogin(client, iss);
  } else if (cmd == "CHAR_LIST") {
    handleCharList(client);
  } else if (cmd == "CHAR_CREATE") {
    handleCharCreate(client, iss);
  } else if (cmd == "CHAR_SELECT") {
    handleCharSelect(client, iss, world);
  } else if (cmd == "MOVE") {
    handleMove(client, iss, world);
  }
  // Unknown/out-of-state commands are silently ignored at this stage — see
  // docs/NETWORKING.md. Each handler below independently checks that the
  // client is in the right state (e.g. MOVE requires entityId != 0), so an
  // out-of-order command from a misbehaving client is a no-op, not a crash.
}

bool TcpServer::allowAuthAttempt(Client& client) {
  const auto now = std::chrono::steady_clock::now();
  if (now - client.lastAuthAttempt < kAuthAttemptInterval) {
    // Rejected before any scrypt work — this is the point of the throttle.
    sendLine(client, "AUTH_FAIL rate_limited");
    recordAuthFailure(client);
    return false;
  }
  client.lastAuthAttempt = now;
  return true;
}

void TcpServer::recordAuthFailure(Client& client) {
  if (++client.failedAuthAttempts >= kMaxFailedAuthAttempts) {
    client.fatal = true;
  }
}

void TcpServer::handleRegister(Client& client, std::istringstream& args) {
  if (client.accountId) return;  // already authenticated on this connection
  if (!allowAuthAttempt(client)) return;

  std::string username;
  args >> username;
  const std::string password = readRestOfLine(args);

  if (!isValidUsername(username) || !isValidPassword(password)) {
    sendLine(client, "AUTH_FAIL invalid_input");
    recordAuthFailure(client);
    return;
  }

  // Checked before hashing so a taken name costs a DB lookup, not a scrypt
  // run. Single-threaded server, so no race between this check and the
  // insert below; createAccount() still relies on the UNIQUE constraint.
  if (db_.findAccountByUsername(username)) {
    sendLine(client, "AUTH_FAIL username_taken");
    recordAuthFailure(client);
    return;
  }

  auth::PasswordHash hash;
  if (!auth::hashPassword(password, hash)) {
    sendLine(client, "AUTH_FAIL server_error");
    return;
  }

  const auto accountId = db_.createAccount(username, hash);
  if (!accountId) {
    sendLine(client, "AUTH_FAIL username_taken");
    recordAuthFailure(client);
    return;
  }

  client.accountId = *accountId;
  sendLine(client, "AUTH_OK");
}

void TcpServer::handleLogin(Client& client, std::istringstream& args) {
  if (client.accountId) return;
  if (!allowAuthAttempt(client)) return;

  std::string username;
  args >> username;
  const std::string password = readRestOfLine(args);

  // The reply is identical for "no such user" and "wrong password". That
  // does not make usernames secret: REGISTER answers username_taken, and an
  // unknown user is rejected without running scrypt (measurably faster).
  // Accepted at this stage — see docs/KNOWN_ISSUES.md.
  const auto account = db_.findAccountByUsername(username);
  if (!account || !auth::verifyPassword(password, account->passwordHash)) {
    sendLine(client, "AUTH_FAIL bad_credentials");
    recordAuthFailure(client);
    return;
  }

  client.accountId = account->id;
  sendLine(client, "AUTH_OK");
}

void TcpServer::handleCharList(Client& client) {
  if (!client.accountId) return;

  const auto characters = db_.listCharacters(*client.accountId);
  std::string line = "CHARS";
  for (const auto& ch : characters) {
    line += " " + std::to_string(ch.id) + ":" + ch.name;
  }
  sendLine(client, line);
}

void TcpServer::handleCharCreate(Client& client, std::istringstream& args) {
  if (!client.accountId) return;

  const std::string name = readRestOfLine(args);
  if (!isValidCharacterName(name)) {
    sendLine(client, "CHAR_CREATE_FAIL invalid_name");
    return;
  }

  const auto characterId = db_.createCharacter(*client.accountId, name);
  if (!characterId) {
    sendLine(client, "CHAR_CREATE_FAIL name_taken");
    return;
  }
  sendLine(client, "CHAR_CREATED " + std::to_string(*characterId) + " " + name);
}

void TcpServer::handleCharSelect(Client& client, std::istringstream& args, World& world) {
  if (!client.accountId || client.entityId != 0) return;  // already playing

  db::CharacterId characterId;
  if (!(args >> characterId)) {
    sendLine(client, "CHAR_SELECT_FAIL invalid_id");
    return;
  }

  const auto record = db_.getOwnedCharacter(*client.accountId, characterId);
  if (!record) {
    // Covers both "doesn't exist" and "belongs to another account" — a
    // client must never be able to distinguish those (see
    // docs/ARCHITECTURE.md: never trust a client-provided id on its own).
    sendLine(client, "CHAR_SELECT_FAIL not_found");
    return;
  }

  client.characterId = record->id;
  client.entityId = world.addEntity(record->x, record->y);
  sendLine(client, "WELCOME " + std::to_string(client.entityId));

  // Snapshot of everyone already in the world: broadcastDirty() only sends
  // entities that changed this tick, so without this a joining (or
  // reconnecting) player would not see anyone standing still. The joining
  // entity itself is marked dirty by addEntity() and goes out next tick.
  for (const auto& [id, entity] : world.entities()) {
    if (id == client.entityId) continue;
    sendLine(client, "POS " + std::to_string(id) + " " + std::to_string(entity.x) + " " +
                         std::to_string(entity.y));
  }
}

void TcpServer::handleMove(Client& client, std::istringstream& args, World& world) {
  if (client.entityId == 0) return;  // not playing yet

  float dx = 0.0f, dy = 0.0f;
  if (args >> dx >> dy) {
    world.applyMove(client.entityId, dx, dy);
  }
}

void TcpServer::removeClient(std::size_t index, World& world) {
  Client& c = clients_[index];

  if (c.entityId != 0) {
    saveCharacter(c, world);
    world.removeEntity(c.entityId);
    broadcastLine("LEAVE " + std::to_string(c.entityId));
    std::cout << "Client disconnected: entity " << c.entityId << "\n";
  } else {
    std::cout << "Client disconnected before selecting a character, fd " << c.fd << "\n";
  }

  if (c.ssl) {
    SSL_shutdown(c.ssl);
    SSL_free(c.ssl);
  }
  ::close(c.fd);

  // Guard against self-move-assignment when `index` is already the last
  // element: `clients_[index] = std::move(clients_.back())` with
  // index == size()-1 would move-assign a Client onto itself, which for a
  // struct holding std::string members is not safe (libstdc++'s move
  // assignment does not check for aliasing) — this was a real bug found
  // via testing (see docs/CHANGELOG.md): it corrupted the heap and caused
  // unrelated, already-connected clients to fail moments later.
  if (index != clients_.size() - 1) {
    clients_[index] = std::move(clients_.back());
  }
  clients_.pop_back();
}

void TcpServer::broadcastDirty(World& world) {
  for (EntityId id : world.dirty()) {
    auto it = world.entities().find(id);
    if (it == world.entities().end()) continue;
    broadcastLine("POS " + std::to_string(id) + " " + std::to_string(it->second.x) + " " +
                  std::to_string(it->second.y));
  }
  world.clearDirty();
}

void TcpServer::sendLine(Client& client, const std::string& line) {
  if (client.fatal) return;
  client.outbuf += line;
  client.outbuf += "\n";
  flushOutbuf(client);
  if (client.outbuf.size() > kMaxOutboundBufferBytes) {
    client.fatal = true;  // peer isn't reading — drop rather than buffer forever
  }
}

void TcpServer::broadcastLine(const std::string& line) {
  // World state (POS/LEAVE) goes only to clients that are in the world.
  // Anyone can complete a TLS handshake without an account, so sending it
  // to every connection would let an unauthenticated observer track every
  // player's movement.
  for (auto& c : clients_) {
    if (c.entityId != 0) sendLine(c, line);
  }
}

void TcpServer::flushOutbuf(Client& client) {
  while (!client.outbuf.empty()) {
    ERR_clear_error();  // see serviceHandshake() for why this is required
    const int n = SSL_write(client.ssl, client.outbuf.data(),
                             static_cast<int>(client.outbuf.size()));
    if (n > 0) {
      client.outbuf.erase(0, static_cast<std::size_t>(n));
      continue;
    }
    const int err = SSL_get_error(client.ssl, n);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      break;  // retry next tick — see runLoop's unconditional readClients() call
    }
    client.fatal = true;
    client.outbuf.clear();
    break;
  }
}

}  // namespace game
