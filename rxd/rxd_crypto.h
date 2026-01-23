// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_CRYPTO_H
#define RXD_CRYPTO_H

#include <vector>
#include <cstdint>
#include <array>

namespace rxd {
namespace crypto {

/**
 * SHA-256 hash function
 * @param data Input data
 * @return 32-byte hash
 */
std::vector<uint8_t> SHA256(const std::vector<uint8_t>& data);
std::vector<uint8_t> SHA256(const uint8_t* data, size_t len);

/**
 * Double SHA-256 (SHA256d) - used for HASH256
 * @param data Input data
 * @return 32-byte hash
 */
std::vector<uint8_t> Hash256(const std::vector<uint8_t>& data);
std::vector<uint8_t> Hash256(const uint8_t* data, size_t len);

/**
 * RIPEMD-160 hash function
 * @param data Input data
 * @return 20-byte hash
 */
std::vector<uint8_t> RIPEMD160(const std::vector<uint8_t>& data);
std::vector<uint8_t> RIPEMD160(const uint8_t* data, size_t len);

/**
 * HASH160 = RIPEMD160(SHA256(data))
 * @param data Input data
 * @return 20-byte hash
 */
std::vector<uint8_t> Hash160(const std::vector<uint8_t>& data);
std::vector<uint8_t> Hash160(const uint8_t* data, size_t len);

/**
 * SHA-512/256 hash function (Radiant-specific)
 * @param data Input data
 * @return 32-byte hash
 */
std::vector<uint8_t> SHA512_256(const std::vector<uint8_t>& data);
std::vector<uint8_t> SHA512_256(const uint8_t* data, size_t len);

/**
 * HASH512_256 = SHA512_256(SHA512_256(data)) (Radiant-specific)
 * @param data Input data
 * @return 32-byte hash
 */
std::vector<uint8_t> Hash512_256(const std::vector<uint8_t>& data);
std::vector<uint8_t> Hash512_256(const uint8_t* data, size_t len);

/**
 * SHA-1 hash function (legacy, for OP_SHA1)
 * @param data Input data
 * @return 20-byte hash
 */
std::vector<uint8_t> SHA1(const std::vector<uint8_t>& data);
std::vector<uint8_t> SHA1(const uint8_t* data, size_t len);

} // namespace crypto
} // namespace rxd

#endif // RXD_CRYPTO_H
