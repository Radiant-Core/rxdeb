// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_signature.h"
#include "rxd_crypto.h"

#include <cstring>
#include <algorithm>

// Use btcdeb's secp256k1 library
#include "../secp256k1/include/secp256k1.h"

namespace rxd {

namespace {

// Global secp256k1 context
secp256k1_context* GetSecp256k1Context() {
    static secp256k1_context* ctx = secp256k1_context_create(
        SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    return ctx;
}

// Serialize a 32-bit value in little-endian
void WriteLE32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back(val & 0xff);
    out.push_back((val >> 8) & 0xff);
    out.push_back((val >> 16) & 0xff);
    out.push_back((val >> 24) & 0xff);
}

// Serialize a 64-bit value in little-endian
void WriteLE64(std::vector<uint8_t>& out, int64_t val) {
    uint64_t u = static_cast<uint64_t>(val);
    for (int i = 0; i < 8; i++) {
        out.push_back(u & 0xff);
        u >>= 8;
    }
}

// Serialize a variable-length integer
void WriteVarInt(std::vector<uint8_t>& out, uint64_t val) {
    if (val < 0xfd) {
        out.push_back(static_cast<uint8_t>(val));
    } else if (val <= 0xffff) {
        out.push_back(0xfd);
        out.push_back(val & 0xff);
        out.push_back((val >> 8) & 0xff);
    } else if (val <= 0xffffffff) {
        out.push_back(0xfe);
        WriteLE32(out, static_cast<uint32_t>(val));
    } else {
        out.push_back(0xff);
        WriteLE64(out, static_cast<int64_t>(val));
    }
}

// Compute hash of all prevouts (for BIP143)
std::vector<uint8_t> GetPrevoutsHash(const CRxdTx& tx) {
    std::vector<uint8_t> data;
    for (const auto& in : tx.GetInputs()) {
        // Prevout txid (32 bytes, reversed)
        auto txid = in.GetPrevTxId();
        data.insert(data.end(), txid.begin(), txid.end());
        // Prevout index (4 bytes)
        WriteLE32(data, in.GetPrevIndex());
    }
    return crypto::Hash256(data);
}

// Compute hash of all sequences (for BIP143)
std::vector<uint8_t> GetSequenceHash(const CRxdTx& tx) {
    std::vector<uint8_t> data;
    for (const auto& in : tx.GetInputs()) {
        WriteLE32(data, in.GetSequence());
    }
    return crypto::Hash256(data);
}

// Compute hash of all outputs (for BIP143)
std::vector<uint8_t> GetOutputsHash(const CRxdTx& tx) {
    std::vector<uint8_t> data;
    for (const auto& out : tx.GetOutputs()) {
        WriteLE64(data, out.GetValue());
        auto script = out.GetScript().data();
        WriteVarInt(data, script.size());
        data.insert(data.end(), script.begin(), script.end());
    }
    return crypto::Hash256(data);
}

} // anonymous namespace

std::vector<uint8_t> SignatureHash(
    const CRxdTx& tx,
    unsigned int nIn,
    const CRxdScript& scriptCode,
    int64_t amount,
    uint32_t nHashType)
{
    // BIP143 signature hash algorithm with FORKID
    // https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki
    
    if (nIn >= tx.GetInputs().size()) {
        // Invalid input index - return all zeros
        return std::vector<uint8_t>(32, 0);
    }
    
    uint32_t nBaseSigHash = GetBaseSigHashType(nHashType);
    bool fAnyoneCanPay = HasAnyoneCanPay(nHashType);
    bool fSingle = (nBaseSigHash == SIGHASH_SINGLE);
    bool fNone = (nBaseSigHash == SIGHASH_NONE);
    
    std::vector<uint8_t> preimage;
    
    // 1. nVersion (4 bytes)
    WriteLE32(preimage, tx.GetVersion());
    
    // 2. hashPrevouts (32 bytes)
    if (!fAnyoneCanPay) {
        auto hash = GetPrevoutsHash(tx);
        preimage.insert(preimage.end(), hash.begin(), hash.end());
    } else {
        preimage.insert(preimage.end(), 32, 0);
    }
    
    // 3. hashSequence (32 bytes)
    if (!fAnyoneCanPay && !fSingle && !fNone) {
        auto hash = GetSequenceHash(tx);
        preimage.insert(preimage.end(), hash.begin(), hash.end());
    } else {
        preimage.insert(preimage.end(), 32, 0);
    }
    
    // 4. outpoint (36 bytes)
    const auto& input = tx.GetInputs()[nIn];
    auto txid = input.GetPrevTxId();
    preimage.insert(preimage.end(), txid.begin(), txid.end());
    WriteLE32(preimage, input.GetPrevIndex());
    
    // 5. scriptCode (varint + data)
    auto script = scriptCode.data();
    WriteVarInt(preimage, script.size());
    preimage.insert(preimage.end(), script.begin(), script.end());
    
    // 6. amount (8 bytes)
    WriteLE64(preimage, amount);
    
    // 7. nSequence (4 bytes)
    WriteLE32(preimage, input.GetSequence());
    
    // 8. hashOutputs (32 bytes)
    if (!fSingle && !fNone) {
        auto hash = GetOutputsHash(tx);
        preimage.insert(preimage.end(), hash.begin(), hash.end());
    } else if (fSingle && nIn < tx.GetOutputs().size()) {
        // Hash only the output at the same index
        std::vector<uint8_t> data;
        const auto& out = tx.GetOutputs()[nIn];
        WriteLE64(data, out.GetValue());
        auto outScript = out.GetScript().data();
        WriteVarInt(data, outScript.size());
        data.insert(data.end(), outScript.begin(), outScript.end());
        auto hash = crypto::Hash256(data);
        preimage.insert(preimage.end(), hash.begin(), hash.end());
    } else {
        preimage.insert(preimage.end(), 32, 0);
    }
    
    // 9. nLockTime (4 bytes)
    WriteLE32(preimage, tx.GetLockTime());
    
    // 10. nHashType (4 bytes) - includes FORKID bits
    // Radiant FORKID = 0 (same as BCH mainnet originally)
    uint32_t forkValue = 0;
    WriteLE32(preimage, nHashType | (forkValue << 8));
    
    // Double SHA256 the preimage
    return crypto::Hash256(preimage);
}

bool IsValidSignatureEncoding(const std::vector<uint8_t>& sig) {
    // Minimum and maximum size constraints (9 bytes min for valid DER, 73 max with sighash)
    if (sig.size() < 9 || sig.size() > 73) return false;
    
    // A signature is of type 0x30 (compound)
    if (sig[0] != 0x30) return false;
    
    // Length of the signature - can be with or without sighash byte
    // sig[1] should equal total length minus header (2 bytes) and optional sighash (0 or 1 byte)
    size_t expectedLen = sig[1];
    if (expectedLen != sig.size() - 2 && expectedLen != sig.size() - 3) return false;
    
    // R value
    if (sig[2] != 0x02) return false;
    
    unsigned int lenR = sig[3];
    if (lenR == 0 || lenR > 33) return false;
    if (5 + lenR >= sig.size()) return false;
    
    // S value
    if (sig[4 + lenR] != 0x02) return false;
    
    unsigned int lenS = sig[5 + lenR];
    if (lenS == 0 || lenS > 33) return false;
    // Total DER structure: 6 + lenR + lenS bytes, plus optional sighash byte
    size_t derLen = 6 + lenR + lenS;
    if (derLen != sig.size() && derLen != sig.size() - 1) return false;
    
    // Check for negative R and S
    if (sig[4] & 0x80) return false;
    if (sig[6 + lenR] & 0x80) return false;
    
    // Check for unnecessary leading zeros in R
    if (lenR > 1 && sig[4] == 0 && !(sig[5] & 0x80)) return false;
    
    // Check for unnecessary leading zeros in S
    if (lenS > 1 && sig[6 + lenR] == 0 && !(sig[7 + lenR] & 0x80)) return false;
    
    return true;
}

bool IsValidPubKey(const std::vector<uint8_t>& pubkey) {
    if (pubkey.size() == 33) {
        // Compressed: 02 or 03 prefix
        return pubkey[0] == 0x02 || pubkey[0] == 0x03;
    }
    if (pubkey.size() == 65) {
        // Uncompressed: 04 prefix
        return pubkey[0] == 0x04;
    }
    return false;
}

uint32_t GetSigHashType(const std::vector<uint8_t>& sig) {
    if (sig.empty()) return 0;
    return sig.back();
}

std::vector<uint8_t> StripSigHashType(const std::vector<uint8_t>& sig) {
    if (sig.empty()) return {};
    return std::vector<uint8_t>(sig.begin(), sig.end() - 1);
}

bool VerifySignature(
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& sig,
    const std::vector<uint8_t>& hash)
{
    if (!IsValidPubKey(pubkey) || hash.size() != 32) {
        return false;
    }
    
    // Strip the sighash byte if present
    std::vector<uint8_t> derSig = sig;
    if (!derSig.empty() && !IsValidSignatureEncoding(derSig)) {
        // Try removing sighash byte
        derSig = StripSigHashType(sig);
    }
    
    if (!IsValidSignatureEncoding(derSig)) {
        return false;
    }
    
    secp256k1_context* ctx = GetSecp256k1Context();
    
    // Parse the public key
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_parse(ctx, &pk, pubkey.data(), pubkey.size())) {
        return false;
    }
    
    // Parse the signature (DER format)
    secp256k1_ecdsa_signature sigObj;
    if (!secp256k1_ecdsa_signature_parse_der(ctx, &sigObj, derSig.data(), derSig.size())) {
        return false;
    }
    
    // Normalize the signature (low-S)
    secp256k1_ecdsa_signature_normalize(ctx, &sigObj, &sigObj);
    
    // Verify
    return secp256k1_ecdsa_verify(ctx, &sigObj, hash.data(), &pk) == 1;
}

bool VerifySchnorrSignature(
    const std::vector<uint8_t>& pubkey,
    const std::vector<uint8_t>& sig,
    const std::vector<uint8_t>& hash)
{
    // Schnorr signatures are 64 bytes, pubkey is 32 bytes (x-only)
    if (pubkey.size() != 32 || sig.size() != 64 || hash.size() != 32) {
        return false;
    }
    
    // TODO: Implement Schnorr verification when supported
    // This requires secp256k1_schnorrsig_verify from libsecp256k1-zkp
    // or the schnorrsig module in newer libsecp256k1
    
    return false;
}

// SignatureChecker implementation

SignatureChecker::SignatureChecker(const CRxdTx& tx, unsigned int nIn, int64_t amount)
    : tx_(tx), nIn_(nIn), amount_(amount)
{
}

bool SignatureChecker::CheckSig(
    const std::vector<uint8_t>& sig,
    const std::vector<uint8_t>& pubkey,
    const CRxdScript& scriptCode) const
{
    if (sig.empty()) {
        return false;
    }
    
    // Get sighash type from signature
    uint32_t nHashType = GetSigHashType(sig);
    
    // Radiant requires FORKID
    if (!HasForkId(nHashType)) {
        return false;
    }
    
    // Compute the signature hash
    auto hash = SignatureHash(tx_, nIn_, scriptCode, amount_, nHashType);
    
    // Verify the signature
    return VerifySignature(pubkey, sig, hash);
}

bool SignatureChecker::CheckLockTime(int64_t nLockTime) const {
    // Check that the locktime is satisfied
    int64_t txLockTime = static_cast<int64_t>(tx_.GetLockTime());
    
    // Locktime type must match (both block height or both timestamp)
    if ((txLockTime < 500000000 && nLockTime >= 500000000) ||
        (txLockTime >= 500000000 && nLockTime < 500000000)) {
        return false;
    }
    
    // Transaction locktime must be >= required locktime
    if (txLockTime < nLockTime) {
        return false;
    }
    
    // Sequence must not be final
    if (nIn_ < tx_.GetInputs().size()) {
        if (tx_.GetInputs()[nIn_].GetSequence() == 0xffffffff) {
            return false;
        }
    }
    
    return true;
}

bool SignatureChecker::CheckSequence(int64_t nSequence) const {
    // Relative locktime check (BIP68)
    if (nIn_ >= tx_.GetInputs().size()) {
        return false;
    }
    
    // Transaction version must be >= 2
    if (tx_.GetVersion() < 2) {
        return false;
    }
    
    uint32_t txSequence = tx_.GetInputs()[nIn_].GetSequence();
    
    // Sequence disable flag
    if (txSequence & (1 << 31)) {
        return false;
    }
    
    // Type flag (bit 22): 0 = blocks, 1 = time
    bool fTypeTx = (txSequence & (1 << 22)) != 0;
    bool fTypeReq = (nSequence & (1 << 22)) != 0;
    
    if (fTypeTx != fTypeReq) {
        return false;
    }
    
    // Mask out type flag and compare values
    uint32_t nMask = 0x0000ffff;
    if ((txSequence & nMask) < (static_cast<uint32_t>(nSequence) & nMask)) {
        return false;
    }
    
    return true;
}

// DummySignatureChecker implementation

CRxdTx DummySignatureChecker::dummyTx_;

DummySignatureChecker::DummySignatureChecker()
    : SignatureChecker(dummyTx_, 0, 0)
{
}

bool DummySignatureChecker::CheckSig(
    const std::vector<uint8_t>& sig,
    const std::vector<uint8_t>& pubkey,
    const CRxdScript& scriptCode) const
{
    // Always return true for testing
    return !sig.empty() && !pubkey.empty();
}

} // namespace rxd
