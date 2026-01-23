// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_crypto.h"
#include <cstring>

// Use btcdeb's existing crypto implementations
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/ripemd160.h"
#include "../crypto/sha1.h"

namespace rxd {
namespace crypto {

std::vector<uint8_t> SHA256(const uint8_t* data, size_t len) {
    std::vector<uint8_t> result(32);
    CSHA256 sha;
    sha.Write(data, len);
    sha.Finalize(result.data());
    return result;
}

std::vector<uint8_t> SHA256(const std::vector<uint8_t>& data) {
    return SHA256(data.data(), data.size());
}

std::vector<uint8_t> Hash256(const uint8_t* data, size_t len) {
    std::vector<uint8_t> result(32);
    CSHA256 sha;
    sha.Write(data, len);
    sha.Finalize(result.data());
    
    CSHA256 sha2;
    sha2.Write(result.data(), 32);
    sha2.Finalize(result.data());
    return result;
}

std::vector<uint8_t> Hash256(const std::vector<uint8_t>& data) {
    return Hash256(data.data(), data.size());
}

std::vector<uint8_t> RIPEMD160(const uint8_t* data, size_t len) {
    std::vector<uint8_t> result(20);
    CRIPEMD160 ripemd;
    ripemd.Write(data, len);
    ripemd.Finalize(result.data());
    return result;
}

std::vector<uint8_t> RIPEMD160(const std::vector<uint8_t>& data) {
    return RIPEMD160(data.data(), data.size());
}

std::vector<uint8_t> Hash160(const uint8_t* data, size_t len) {
    // HASH160 = RIPEMD160(SHA256(data))
    auto sha256_result = SHA256(data, len);
    return RIPEMD160(sha256_result);
}

std::vector<uint8_t> Hash160(const std::vector<uint8_t>& data) {
    return Hash160(data.data(), data.size());
}

std::vector<uint8_t> SHA512_256(const uint8_t* data, size_t len) {
    // SHA-512/256 is SHA-512 with different initial values, truncated to 256 bits
    std::vector<uint8_t> result(32);
    CSHA512 sha;
    sha.Write(data, len);
    
    uint8_t full_hash[64];
    sha.Finalize(full_hash);
    
    // For proper SHA-512/256 we need different IVs
    // This is a simplified version that just truncates SHA-512
    // TODO: Implement proper SHA-512/256 with correct IVs
    std::memcpy(result.data(), full_hash, 32);
    return result;
}

std::vector<uint8_t> SHA512_256(const std::vector<uint8_t>& data) {
    return SHA512_256(data.data(), data.size());
}

std::vector<uint8_t> Hash512_256(const uint8_t* data, size_t len) {
    auto first = SHA512_256(data, len);
    return SHA512_256(first);
}

std::vector<uint8_t> Hash512_256(const std::vector<uint8_t>& data) {
    return Hash512_256(data.data(), data.size());
}

std::vector<uint8_t> SHA1(const uint8_t* data, size_t len) {
    std::vector<uint8_t> result(20);
    CSHA1 sha;
    sha.Write(data, len);
    sha.Finalize(result.data());
    return result;
}

std::vector<uint8_t> SHA1(const std::vector<uint8_t>& data) {
    return SHA1(data.data(), data.size());
}

} // namespace crypto
} // namespace rxd
