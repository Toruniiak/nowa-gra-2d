#pragma once

#include <openssl/ssl.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "db/database.hpp"
#include "net/tls.hpp"
#include "world/world.hpp"

namespace game {

// TCP+TLS server: accepts connections, performs the TLS handshake, parses
// the line-based protocol documented in docs/NETWORKING.md (now including
// REGISTER/LOGIN/CHAR_LIST/CHAR_CREATE/CHAR_SELECT — see that file for the
// full state machine), applies validated intents to the World, and
// broadcasts resulting state changes. Single-threaded, select()-based —
// deliberately simple for this stage of the project; see docs/NETWORKING.md
// "Do zdecydowania" for what will need to change before this scales.
class TcpServer {
 public:
  TcpServer(uint16_t port, net::TlsContext& tls, db::Database& database);
  ~TcpServer();

  // Binds and starts listening. Returns false on failure (logs to stderr).
  bool start();

  // Blocking loop: services network I/O and calls world.applyMove() for
  // validated intents, broadcasting position updates once per tick.
  // tickIntervalMs controls both the select() timeout and broadcast rate.
  // Returns once `stop` becomes non-zero (set from a SIGINT/SIGTERM
  // handler), after saving every in-world character's position.
  void runLoop(World& world, int tickIntervalMs, const volatile std::sig_atomic_t& stop);

 private:
  struct Client {
    int fd = -1;
    SSL* ssl = nullptr;
    bool handshakeDone = false;
    bool fatal = false;  // set when the connection must be dropped
    std::chrono::steady_clock::time_point connectedAt = std::chrono::steady_clock::now();

    std::optional<db::AccountId> accountId;      // set once LOGIN/REGISTER succeeds
    std::optional<db::CharacterId> characterId;  // set once CHAR_SELECT succeeds
    EntityId entityId = 0;                       // 0 = not yet playing

    // Auth throttling — scrypt blocks the single-threaded loop for ~50 ms,
    // so unthrottled LOGIN spam from one connection would freeze the
    // server for every player. See docs/KNOWN_ISSUES.md.
    std::chrono::steady_clock::time_point lastAuthAttempt{};
    int failedAuthAttempts = 0;

    std::string inbuf;
    std::string outbuf;
  };

  uint16_t port_;
  net::TlsContext& tls_;
  db::Database& db_;
  int listenFd_ = -1;
  std::vector<Client> clients_;

  void acceptNew();
  void readClients(World& world);
  void serviceHandshake(Client& client);
  void serviceRead(Client& client, World& world);
  void removeClient(std::size_t index, World& world);
  void saveCharacter(const Client& client, World& world);
  void broadcastDirty(World& world);

  void sendLine(Client& client, const std::string& line);
  void broadcastLine(const std::string& line);
  void flushOutbuf(Client& client);

  bool allowAuthAttempt(Client& client);
  void recordAuthFailure(Client& client);

  void handleLine(Client& client, const std::string& line, World& world);
  void handleRegister(Client& client, std::istringstream& args);
  void handleLogin(Client& client, std::istringstream& args);
  void handleCharList(Client& client);
  void handleCharCreate(Client& client, std::istringstream& args);
  void handleCharSelect(Client& client, std::istringstream& args, World& world);
  void handleMove(Client& client, std::istringstream& args, World& world);
};

}  // namespace game
