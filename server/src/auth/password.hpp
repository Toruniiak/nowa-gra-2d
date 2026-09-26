#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace game::auth {

// Password storage: scrypt (OpenSSL EVP_PBE_scrypt) with a random per-account
// salt. Never store or compare plaintext passwords — see CLAUDE.md security
// rules and docs/KNOWN_ISSUES.md. Chosen over PBKDF2 because it is
// memory-hard (more resistant to GPU/ASIC cracking); chosen over Argon2id
// (the current OWASP-preferred choice) because OpenSSL — already a
// dependency for transport encryption — implements scrypt natively, so this
// avoids adding a second crypto library for one function.
struct PasswordHash {
  std::vector<uint8_t> salt;  // random, kSaltBytes long
  std::vector<uint8_t> hash;  // derived key, kHashBytes long
};

constexpr int kSaltBytes = 16;
constexpr int kHashBytes = 32;

// scrypt cost parameters. N=16384 (2^14), r=8, p=1: comparable to widely used
// interactive-login presets (~16-32 MB memory, low tens of ms on modern
// hardware). Revisit under real load measurements, not preemptively.
constexpr uint64_t kScryptN = 16384;
constexpr uint32_t kScryptR = 8;
constexpr uint32_t kScryptP = 1;

// Derives a new salt + hash for the given plaintext password.
// Returns false on internal OpenSSL failure (caller must treat as fatal for
// that request, never fall back to storing plaintext).
bool hashPassword(const std::string& plaintext, PasswordHash& out);

// Re-derives the hash for `plaintext` using `stored.salt` and compares it to
// `stored.hash` in constant time (CRYPTO_memcmp). Returns false on mismatch
// or internal failure — callers must not distinguish the two in user-facing
// responses (see docs/NETWORKING.md, AUTH_FAIL is generic on login).
bool verifyPassword(const std::string& plaintext, const PasswordHash& stored);

}  // namespace game::auth
