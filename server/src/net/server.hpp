#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "world/world.hpp"

namespace game {

// Minimal TCP server: accepts connections, parses the line-based protocol
// documented in docs/NETWORKING.md, applies validated intents to the World,
// and broadcasts resulting state changes. Single-threaded, select()-based —
// deliberately simple for this stage of the project; see docs/NETWORKING.md
// "Do zdecydowania" for what will need to change before this scales.
class TcpServer {
 public:
  explicit TcpServer(uint16_t port);
  ~TcpServer();

  // Binds and starts listening. Returns false on failure (logs to stderr).
  bool start();

  // Blocking loop: services network I/O and calls world.applyMove() for
  // validated intents, broadcasting position updates once per tick.
  // tickIntervalMs controls both the select() timeout and broadcast rate.
  void runLoop(World& world, int tickIntervalMs);

 private:
  struct Client {
    int fd = -1;
    EntityId entityId = 0;
    std::string inbuf;
  };

  uint16_t port_;
  int listenFd_ = -1;
  std::vector<Client> clients_;

  void acceptNew(World& world);
  void readClients(World& world);
  void removeClient(std::size_t index, World& world);
  void broadcastDirty(World& world);
  void sendLine(int fd, const std::string& line) const;
  void broadcastLine(const std::string& line) const;
  void handleLine(Client& client, const std::string& line, World& world);
};

}  // namespace game
