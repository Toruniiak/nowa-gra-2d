#include <cstdlib>
#include <iostream>

#include "net/server.hpp"
#include "world/world.hpp"

int main(int argc, char** argv) {
  uint16_t port = 7777;
  if (argc > 1) {
    port = static_cast<uint16_t>(std::atoi(argv[1]));
  }

  game::World world;
  game::TcpServer server(port);

  if (!server.start()) {
    std::cerr << "Failed to start server on port " << port << "\n";
    return EXIT_FAILURE;
  }

  constexpr int kTickIntervalMs = 50;  // 20 Hz — starting point, see docs/ARCHITECTURE.md
  server.runLoop(world, kTickIntervalMs);
  return EXIT_SUCCESS;
}
