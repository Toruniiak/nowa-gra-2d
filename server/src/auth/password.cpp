#include "auth/password.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace game::auth {

bool hashPassword(const std::string& plaintext, PasswordHash& out) {
  out.salt.resize(kSaltBytes);
  if (RAND_bytes(out.salt.data(), kSaltBytes) != 1) {
    return false;
  }

  out.hash.resize(kHashBytes);
  const int rc = EVP_PBE_scrypt(plaintext.data(), plaintext.size(), out.salt.data(),
                                 out.salt.size(), kScryptN, kScryptR, kScryptP,
                                 /*maxmem=*/0, out.hash.data(), out.hash.size());
  return rc == 1;
}

bool verifyPassword(const std::string& plaintext, const PasswordHash& stored) {
  if (stored.salt.size() != static_cast<size_t>(kSaltBytes) ||
      stored.hash.size() != static_cast<size_t>(kHashBytes)) {
    return false;
  }

  std::vector<uint8_t> candidate(kHashBytes);
  const int rc = EVP_PBE_scrypt(plaintext.data(), plaintext.size(), stored.salt.data(),
                                 stored.salt.size(), kScryptN, kScryptR, kScryptP,
                                 /*maxmem=*/0, candidate.data(), candidate.size());
  if (rc != 1) return false;

  return CRYPTO_memcmp(candidate.data(), stored.hash.data(), kHashBytes) == 0;
}

}  // namespace game::auth
