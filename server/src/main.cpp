#include <csignal>
#include <cstdlib>
#include <iostream>

#include "db/database.hpp"
#include "net/server.hpp"
#include "net/tls.hpp"
#include "world/map.hpp"
#include "world/world.hpp"

namespace {

volatile std::sig_atomic_t g_stopRequested = 0;

void onStopSignal(int) { g_stopRequested = 1; }

void printUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0
            << " [port] [db_path] [cert_path] [key_path] [data_dir]\n"
               "  port:      default 7777\n"
               "  db_path:   default game.db (created if missing)\n"
               "  cert_path: default certs/server.crt (see docs/BUILD.md to generate)\n"
               "  key_path:  default certs/server.key\n"
               "  data_dir:  default ../client/data (tiles.json + maps/start.json)\n";
}

}  // namespace

int main(int argc, char** argv) {
  uint16_t port = 7777;
  std::string dbPath = "game.db";
  std::string certPath = "certs/server.crt";
  std::string keyPath = "certs/server.key";
  std::string dataDir = "../client/data";

  if (argc > 1) port = static_cast<uint16_t>(std::atoi(argv[1]));
  if (argc > 2) dbPath = argv[2];
  if (argc > 3) certPath = argv[3];
  if (argc > 4) keyPath = argv[4];
  if (argc > 5) dataDir = argv[5];
  if (argc > 6) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  // A client that closes its connection can make a subsequent SSL_write()
  // (via the underlying socket write()) raise SIGPIPE, which by default
  // terminates the whole process — not something a broken client should be
  // able to do to every other player. removeClient() already handles a
  // closed connection cleanly once the send fails; this just stops the
  // signal from taking the process down before that happens.
  std::signal(SIGPIPE, SIG_IGN);

  // Ctrl+C / `kill` / service stop: finish the current tick, save every
  // in-world character, then exit (see TcpServer::runLoop).
  std::signal(SIGINT, onStopSignal);
  std::signal(SIGTERM, onStopSignal);

  try {
    game::net::TlsContext tls(certPath, keyPath);
    game::db::Database database(dbPath);
    game::GameMap map(dataDir + "/tiles.json", dataDir + "/maps/start.json");
    std::cout << "Map loaded: " << map.width() << "x" << map.height() << std::endl;
    game::World world(map);
    game::TcpServer server(port, tls, database);

    if (!server.start()) {
      std::cerr << "Failed to start server on port " << port << "\n";
      return EXIT_FAILURE;
    }

    constexpr int kTickIntervalMs = 50;  // 20 Hz — starting point, see docs/ARCHITECTURE.md
    server.runLoop(world, kTickIntervalMs, g_stopRequested);
    return EXIT_SUCCESS;
  } catch (const std::exception& e) {
    std::cerr << "Fatal startup error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
}
