#pragma once

#include <openssl/ssl.h>

#include <string>

namespace game::net {

// Thin RAII wrapper around an OpenSSL server SSL_CTX. Added in Phase 4
// specifically because docs/KNOWN_ISSUES.md requires encrypted transport
// before any real account/password crosses the wire — see
// docs/NETWORKING.md. This is the server's first external dependency
// (OpenSSL); see docs/TECH_STACK.md for why: rolling custom transport
// crypto is explicitly against project security priorities, and OpenSSL is
// the standard, audited choice.
class TlsContext {
 public:
  // Loads a certificate + private key from PEM files. Throws
  // std::runtime_error if either file is missing/invalid, or if the key
  // doesn't match the certificate — a server that can't set up TLS must not
  // silently fall back to plaintext.
  TlsContext(const std::string& certPath, const std::string& keyPath);
  ~TlsContext();

  TlsContext(const TlsContext&) = delete;
  TlsContext& operator=(const TlsContext&) = delete;

  SSL_CTX* raw() const { return ctx_; }

 private:
  SSL_CTX* ctx_ = nullptr;
};

}  // namespace game::net
