// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_SIGNATURE_H
#define RXD_SIGNATURE_H

#include <vector>
#include <cstdint>
#include <string>

#include "rxd_tx.h"
#include "rxd_script.h"

namespace rxd {

// Signature hash types (Bitcoin/Radiant compatible)
enum SigHashType : uint32_t {
    SIGHASH_ALL          = 0x01,
    SIGHASH_NONE         = 0x02,
    SIGHASH_SINGLE       = 0x03,
    SIGHASH_FORKID       = 0x40,  // Radiant uses FORKID
    SIGHASH_ANYONECANPAY = 0x80,
    
    // Common combinations
    SIGHASH_ALL_FORKID          = SIGHASH_ALL | SIGHASH_FORKID,
    SIGHASH_NONE_FORKID         = SIGHASH_NONE | SIGHASH_FORKID,
    SIGHASH_SINGLE_FORKID       = SIGHASH_SINGLE | SIGHASH_FORKID,
    SIGHASH_ALL_ANYONECANPAY    = SIGHASH_ALL | SIGHASH_ANYONECANPAY | SIGHASH_FORKID,
};

/**
 * Get base sighash type (without flags)
 */
inline uint32_t GetBaseSigHashType(uint32_t nHashType) {
    return nHashType & 0x1f;
}

/**
 * Check if FORKID flag is set
 */
inline bool HasForkId(uint32_t nHashType) {
    return (nHashType & SIGHASH_FORKID) != 0;
}

/**
 * Check if ANYONECANPAY flag is set
 */
inline bool HasAnyoneCanPay(uint32_t nHashType) {
    return (nHashType & SIGHASH_ANYONECANPAY) != 0;
}

/**
 * Compute signature hash for a transaction input (BIP143 style with FORKID)
 * 
 * This implements the sighash algorithm used by Radiant (derived from BCH).
 * 
 * @param tx The transaction being signed
 * @param nIn Input index being signed
 * @param scriptCode The script being executed (after OP_CODESEPARATOR)
 * @param amount Value of the input being spent (satoshis)
 * @param nHashType Signature hash type
 * @return 32-byte hash to be signed
 */
std::vector<uint8_t> SignatureHash(
    const CRxdTx& tx,
    unsigned int nIn,
    const CRxdScript& scriptCode,
    int64_t amount,
    uint32_t nHashType);

/**
 * Verify an ECDSA signature
 * 
 * @param pubkey Public key (33 or 65 bytes)
 * @param sig Signature (DER encoded with sighash byte)
 * @param hash 32-byte message hash
 * @return true if signature is valid
 */
bool VerifySignature(
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& sig,
    const std::vector<uint8_t>& hash);

/**
 * Verify a Schnorr signature (if supported)
 * 
 * @param pubkey Public key (32 bytes x-only)
 * @param sig Signature (64 bytes)
 * @param hash 32-byte message hash
 * @return true if signature is valid
 */
bool VerifySchnorrSignature(
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& sig,
    const std::vector<uint8_t>& hash);

/**
 * Check if a signature is valid DER encoding
 */
bool IsValidSignatureEncoding(const std::vector<uint8_t>& sig);

/**
 * Check if a public key is valid
 */
bool IsValidPubKey(const std::vector<uint8_t>& pubkey);

/**
 * Extract sighash type from signature
 */
uint32_t GetSigHashType(const std::vector<uint8_t>& sig);

/**
 * Remove sighash byte from signature
 */
std::vector<uint8_t> StripSigHashType(const std::vector<uint8_t>& sig);

/**
 * Signature checker for script execution
 */
class SignatureChecker {
public:
    SignatureChecker(const CRxdTx& tx, unsigned int nIn, int64_t amount);
    virtual ~SignatureChecker() = default;
    
    /**
     * Check a signature against a public key
     */
    virtual bool CheckSig(
        const std::vector<uint8_t>& sig,
        const std::vector<uint8_t>& pubkey,
        const CRxdScript& scriptCode) const;
    
    /**
     * Check a lock time constraint
     */
    virtual bool CheckLockTime(int64_t nLockTime) const;
    
    /**
     * Check a sequence constraint
     */
    virtual bool CheckSequence(int64_t nSequence) const;
    
    /**
     * Get the transaction being verified
     */
    const CRxdTx& GetTx() const { return tx_; }
    
    /**
     * Get the input index being verified
     */
    unsigned int GetInputIndex() const { return nIn_; }
    
    /**
     * Get the input amount
     */
    int64_t GetAmount() const { return amount_; }

protected:
    const CRxdTx& tx_;
    unsigned int nIn_;
    int64_t amount_;
};

/**
 * Dummy signature checker that always returns true (for testing)
 */
class DummySignatureChecker : public SignatureChecker {
public:
    DummySignatureChecker();
    
    bool CheckSig(
        const std::vector<uint8_t>& sig,
        const std::vector<uint8_t>& pubkey,
        const CRxdScript& scriptCode) const override;
    
    bool CheckLockTime(int64_t nLockTime) const override { return true; }
    bool CheckSequence(int64_t nSequence) const override { return true; }

private:
    static CRxdTx dummyTx_;
};

} // namespace rxd

#endif // RXD_SIGNATURE_H
