#include "net/tls.hpp"

#include <openssl/err.h>

#include <stdexcept>

namespace game::net {

namespace {

std::string lastSslError() {
  char buf[256];
  ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
  return buf;
}

}  // namespace

TlsContext::TlsContext(const std::string& certPath, const std::string& keyPath) {
  ctx_ = SSL_CTX_new(TLS_server_method());
  if (!ctx_) {
    throw std::runtime_error("SSL_CTX_new failed: " + lastSslError());
  }

  // Require TLS 1.2+ — no legacy protocol versions.
  SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);

  if (SSL_CTX_use_certificate_file(ctx_, certPath.c_str(), SSL_FILETYPE_PEM) != 1) {
    SSL_CTX_free(ctx_);
    throw std::runtime_error("Failed to load certificate '" + certPath + "': " + lastSslError());
  }
  if (SSL_CTX_use_PrivateKey_file(ctx_, keyPath.c_str(), SSL_FILETYPE_PEM) != 1) {
    SSL_CTX_free(ctx_);
    throw std::runtime_error("Failed to load private key '" + keyPath + "': " + lastSslError());
  }
  if (SSL_CTX_check_private_key(ctx_) != 1) {
    SSL_CTX_free(ctx_);
    throw std::runtime_error("Certificate/private key mismatch for '" + certPath + "' / '" +
                              keyPath + "'");
  }
}

TlsContext::~TlsContext() {
  if (ctx_) SSL_CTX_free(ctx_);
}

}  // namespace game::net
