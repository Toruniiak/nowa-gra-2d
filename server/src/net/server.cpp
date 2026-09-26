#include "net/server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

namespace game {

TcpServer::TcpServer(uint16_t port) : port_(port) {}

TcpServer::~TcpServer() {
  for (auto& c : clients_) {
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

void TcpServer::runLoop(World& world, int tickIntervalMs) {
  std::cout << "Server listening on port " << port_ << " (tick=" << tickIntervalMs << "ms)\n";
  for (;;) {
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
      acceptNew(world);
    }
    if (ready > 0) {
      readClients(world);
    }

    broadcastDirty(world);
  }
}

void TcpServer::acceptNew(World& world) {
  sockaddr_in clientAddr{};
  socklen_t len = sizeof(clientAddr);
  const int fd = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&clientAddr), &len);
  if (fd < 0) return;

  Client client;
  client.fd = fd;
  client.entityId = world.addEntity();
  clients_.push_back(client);

  sendLine(fd, "WELCOME " + std::to_string(client.entityId));
  std::cout << "Client connected: entity " << client.entityId << "\n";
}

void TcpServer::readClients(World& world) {
  char buf[512];
  for (std::size_t i = 0; i < clients_.size();) {
    Client& c = clients_[i];
    fd_set testSet;
    FD_ZERO(&testSet);
    FD_SET(c.fd, &testSet);
    timeval zero{0, 0};
    if (::select(c.fd + 1, &testSet, nullptr, nullptr, &zero) <= 0 || !FD_ISSET(c.fd, &testSet)) {
      ++i;
      continue;
    }

    const ssize_t n = ::recv(c.fd, buf, sizeof(buf), 0);
    if (n <= 0) {
      removeClient(i, world);
      continue;  // do not advance i: a client shifted into this slot
    }

    c.inbuf.append(buf, static_cast<std::size_t>(n));
    std::size_t pos;
    while ((pos = c.inbuf.find('\n')) != std::string::npos) {
      std::string line = c.inbuf.substr(0, pos);
      c.inbuf.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      handleLine(c, line, world);
    }
    ++i;
  }
}

void TcpServer::handleLine(Client& client, const std::string& line, World& world) {
  std::istringstream iss(line);
  std::string cmd;
  iss >> cmd;
  if (cmd == "MOVE") {
    float dx = 0.0f, dy = 0.0f;
    if (iss >> dx >> dy) {
      world.applyMove(client.entityId, dx, dy);
    }
  }
  // Unknown commands are silently ignored at this stage. Once more intents
  // exist (ATTACK, USE_ITEM, ...), log unrecognized input for diagnostics —
  // never assume malformed input is harmless just because it's unhandled.
}

void TcpServer::removeClient(std::size_t index, World& world) {
  const EntityId id = clients_[index].entityId;
  ::close(clients_[index].fd);
  clients_[index] = clients_.back();
  clients_.pop_back();

  world.removeEntity(id);
  broadcastLine("LEAVE " + std::to_string(id));
  std::cout << "Client disconnected: entity " << id << "\n";
}

void TcpServer::broadcastDirty(World& world) {
  for (EntityId id : world.dirty()) {
    auto it = world.entities().find(id);
    if (it == world.entities().end()) continue;
    std::ostringstream oss;
    oss << "POS " << id << " " << it->second.x << " " << it->second.y;
    broadcastLine(oss.str());
  }
  world.clearDirty();
}

void TcpServer::sendLine(int fd, const std::string& line) const {
  const std::string withNewline = line + "\n";
  ::send(fd, withNewline.data(), withNewline.size(), 0);
}

void TcpServer::broadcastLine(const std::string& line) const {
  for (const auto& c : clients_) {
    sendLine(c.fd, line);
  }
}

}  // namespace game
