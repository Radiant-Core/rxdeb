// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_tx.h"
#include "rxd_script.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

namespace rxd {

// Helper functions for hex encoding/decoding
static std::string BytesToHex(const std::vector<uint8_t>& data) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

static std::vector<uint8_t> FromHex(const std::string& hex) {
    std::vector<uint8_t> result;
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("Invalid hex string length");
    }
    result.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t byte = 0;
        for (int j = 0; j < 2; j++) {
            char c = hex[i + j];
            byte <<= 4;
            if (c >= '0' && c <= '9') byte |= (c - '0');
            else if (c >= 'a' && c <= 'f') byte |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') byte |= (c - 'A' + 10);
            else throw std::runtime_error("Invalid hex character");
        }
        result.push_back(byte);
    }
    return result;
}

// CRxdOutPoint implementation

std::string CRxdOutPoint::ToString() const {
    std::ostringstream ss;
    // Reverse txid for display (Bitcoin convention)
    std::vector<uint8_t> reversed(txid.rbegin(), txid.rend());
    ss << BytesToHex(reversed) << ":" << n;
    return ss.str();
}

std::string CRxdOutPoint::ToHex() const {
    std::vector<uint8_t> result;
    result.insert(result.end(), txid.begin(), txid.end());
    result.push_back(n & 0xff);
    result.push_back((n >> 8) & 0xff);
    result.push_back((n >> 16) & 0xff);
    result.push_back((n >> 24) & 0xff);
    return BytesToHex(result);
}

std::vector<uint8_t> CRxdOutPoint::ToRef() const {
    std::vector<uint8_t> ref;
    ref.reserve(36);
    ref.insert(ref.end(), txid.begin(), txid.end());
    ref.push_back(n & 0xff);
    ref.push_back((n >> 8) & 0xff);
    ref.push_back((n >> 16) & 0xff);
    ref.push_back((n >> 24) & 0xff);
    return ref;
}

CRxdOutPoint CRxdOutPoint::FromRef(const std::vector<uint8_t>& ref) {
    if (ref.size() != 36) {
        throw std::runtime_error("Invalid reference size");
    }
    CRxdOutPoint outpoint;
    outpoint.txid.assign(ref.begin(), ref.begin() + 32);
    outpoint.n = ref[32] | (ref[33] << 8) | (ref[34] << 16) | (ref[35] << 24);
    return outpoint;
}

// CRxdTxIn implementation

std::string CRxdTxIn::ToString() const {
    std::ostringstream ss;
    ss << "CTxIn(" << prevout.ToString() << ", scriptSig=" 
       << scriptSig.ToHex().substr(0, 24) << "...)";
    return ss.str();
}

// CRxdTxOut implementation

std::string CRxdTxOut::ToString() const {
    std::ostringstream ss;
    ss << "CTxOut(nValue=" << nValue << ", scriptPubKey=" 
       << scriptPubKey.ToHex().substr(0, 24) << "...)";
    return ss.str();
}

// CRxdTx implementation

std::vector<uint8_t> CRxdTx::GetHash() const {
    // TODO: Implement proper double SHA256 hash
    // For now return a placeholder
    std::vector<uint8_t> hash(32, 0);
    
    // Simple hash placeholder - in real implementation use SHA256d
    auto serialized = Serialize();
    if (!serialized.empty()) {
        for (size_t i = 0; i < serialized.size(); i++) {
            hash[i % 32] ^= serialized[i];
        }
    }
    
    return hash;
}

std::string CRxdTx::GetHashHex() const {
    auto hash = GetHash();
    // Reverse for display (Bitcoin convention)
    std::reverse(hash.begin(), hash.end());
    return BytesToHex(hash);
}

int64_t CRxdTx::GetValueOut() const {
    int64_t total = 0;
    for (const auto& out : vout) {
        total += out.nValue;
    }
    return total;
}

bool CRxdTx::IsCoinBase() const {
    return vin.size() == 1 && vin[0].prevout.IsNull();
}

std::vector<uint8_t> CRxdTx::Serialize() const {
    std::vector<uint8_t> result;
    
    // Version (4 bytes, little-endian)
    result.push_back(nVersion & 0xff);
    result.push_back((nVersion >> 8) & 0xff);
    result.push_back((nVersion >> 16) & 0xff);
    result.push_back((nVersion >> 24) & 0xff);
    
    // Input count (varint)
    auto WriteVarInt = [&result](uint64_t n) {
        if (n < 0xfd) {
            result.push_back(static_cast<uint8_t>(n));
        } else if (n <= 0xffff) {
            result.push_back(0xfd);
            result.push_back(n & 0xff);
            result.push_back((n >> 8) & 0xff);
        } else if (n <= 0xffffffff) {
            result.push_back(0xfe);
            result.push_back(n & 0xff);
            result.push_back((n >> 8) & 0xff);
            result.push_back((n >> 16) & 0xff);
            result.push_back((n >> 24) & 0xff);
        } else {
            result.push_back(0xff);
            for (int i = 0; i < 8; i++) {
                result.push_back((n >> (8 * i)) & 0xff);
            }
        }
    };
    
    WriteVarInt(vin.size());
    
    // Inputs
    for (const auto& in : vin) {
        // Previous output
        result.insert(result.end(), in.prevout.txid.begin(), in.prevout.txid.end());
        result.push_back(in.prevout.n & 0xff);
        result.push_back((in.prevout.n >> 8) & 0xff);
        result.push_back((in.prevout.n >> 16) & 0xff);
        result.push_back((in.prevout.n >> 24) & 0xff);
        
        // Script length and data
        const auto& scriptData = in.scriptSig.data();
        WriteVarInt(scriptData.size());
        result.insert(result.end(), scriptData.begin(), scriptData.end());
        
        // Sequence
        result.push_back(in.nSequence & 0xff);
        result.push_back((in.nSequence >> 8) & 0xff);
        result.push_back((in.nSequence >> 16) & 0xff);
        result.push_back((in.nSequence >> 24) & 0xff);
    }
    
    // Output count
    WriteVarInt(vout.size());
    
    // Outputs
    for (const auto& out : vout) {
        // Value (8 bytes)
        uint64_t value = static_cast<uint64_t>(out.nValue);
        for (int i = 0; i < 8; i++) {
            result.push_back((value >> (8 * i)) & 0xff);
        }
        
        // Script length and data
        const auto& scriptData = out.scriptPubKey.data();
        WriteVarInt(scriptData.size());
        result.insert(result.end(), scriptData.begin(), scriptData.end());
    }
    
    // Locktime (4 bytes)
    result.push_back(nLockTime & 0xff);
    result.push_back((nLockTime >> 8) & 0xff);
    result.push_back((nLockTime >> 16) & 0xff);
    result.push_back((nLockTime >> 24) & 0xff);
    
    return result;
}

std::string CRxdTx::ToHex() const {
    return BytesToHex(Serialize());
}

CRxdTx CRxdTx::FromHex(const std::string& hex) {
    auto data = rxd::FromHex(hex);
    return Deserialize(data);
}

CRxdTx CRxdTx::Deserialize(const std::vector<uint8_t>& data) {
    return Deserialize(data.data(), data.size());
}

CRxdTx CRxdTx::Deserialize(const uint8_t* data, size_t len) {
    CRxdTx tx;
    size_t pos = 0;
    
    auto ReadLE32 = [&]() -> uint32_t {
        if (pos + 4 > len) throw std::runtime_error("Unexpected end of data");
        uint32_t v = data[pos] | (data[pos+1] << 8) | (data[pos+2] << 16) | (data[pos+3] << 24);
        pos += 4;
        return v;
    };
    
    auto ReadLE64 = [&]() -> uint64_t {
        if (pos + 8 > len) throw std::runtime_error("Unexpected end of data");
        uint64_t v = 0;
        for (int i = 0; i < 8; i++) {
            v |= static_cast<uint64_t>(data[pos + i]) << (8 * i);
        }
        pos += 8;
        return v;
    };
    
    auto ReadVarInt = [&]() -> uint64_t {
        if (pos >= len) throw std::runtime_error("Unexpected end of data");
        uint8_t first = data[pos++];
        if (first < 0xfd) return first;
        if (first == 0xfd) {
            if (pos + 2 > len) throw std::runtime_error("Unexpected end of data");
            uint16_t v = data[pos] | (data[pos+1] << 8);
            pos += 2;
            return v;
        }
        if (first == 0xfe) {
            return ReadLE32();
        }
        return ReadLE64();
    };
    
    auto ReadBytes = [&](size_t n) -> std::vector<uint8_t> {
        if (pos + n > len) throw std::runtime_error("Unexpected end of data");
        std::vector<uint8_t> result(data + pos, data + pos + n);
        pos += n;
        return result;
    };
    
    // Version
    tx.nVersion = static_cast<int32_t>(ReadLE32());
    
    // Inputs
    uint64_t inputCount = ReadVarInt();
    for (uint64_t i = 0; i < inputCount; i++) {
        CRxdTxIn in;
        in.prevout.txid = ReadBytes(32);
        in.prevout.n = ReadLE32();
        uint64_t scriptLen = ReadVarInt();
        auto scriptData = ReadBytes(scriptLen);
        in.scriptSig = CRxdScript(scriptData);
        in.nSequence = ReadLE32();
        tx.vin.push_back(in);
    }
    
    // Outputs
    uint64_t outputCount = ReadVarInt();
    for (uint64_t i = 0; i < outputCount; i++) {
        CRxdTxOut out;
        out.nValue = static_cast<int64_t>(ReadLE64());
        uint64_t scriptLen = ReadVarInt();
        auto scriptData = ReadBytes(scriptLen);
        out.scriptPubKey = CRxdScript(scriptData);
        tx.vout.push_back(out);
    }
    
    // Locktime
    tx.nLockTime = ReadLE32();
    
    return tx;
}

std::string CRxdTx::ToString() const {
    std::ostringstream ss;
    ss << "CTransaction(hash=" << GetHashHex().substr(0, 16) << "..., "
       << "ver=" << nVersion << ", "
       << "vin.size=" << vin.size() << ", "
       << "vout.size=" << vout.size() << ", "
       << "nLockTime=" << nLockTime << ")";
    return ss.str();
}

// CMutableRxdTx implementation

CMutableRxdTx::CMutableRxdTx(const CRxdTx& tx)
    : nVersion(tx.nVersion)
    , vin(tx.vin)
    , vout(tx.vout)
    , nLockTime(tx.nLockTime)
{
}

CRxdTx CMutableRxdTx::ToTx() const {
    CRxdTx tx;
    tx.nVersion = nVersion;
    tx.vin = vin;
    tx.vout = vout;
    tx.nLockTime = nLockTime;
    return tx;
}

std::vector<uint8_t> CMutableRxdTx::GetHash() const {
    return ToTx().GetHash();
}

// SigHash namespace implementation

namespace SigHash {

std::vector<uint8_t> ComputeSigHash(
    const CRxdTx& tx,
    unsigned int inputIndex,
    const CRxdScript& scriptCode,
    int64_t amount,
    uint32_t sigHashType)
{
    // Implement BIP143-style sighash for Radiant (with SIGHASH_FORKID)
    std::vector<uint8_t> result(32, 0);
    
    // Check for SIGHASH_FORKID
    bool useForkId = (sigHashType & 0x40) != 0;
    uint32_t baseType = sigHashType & 0x1f;
    bool anyoneCanPay = (sigHashType & 0x80) != 0;
    
    if (!useForkId) {
        // Legacy sighash - not recommended for Radiant
        // For simplicity, return a placeholder
        return result;
    }
    
    // BIP143 sighash algorithm
    // 1. nVersion
    // 2. hashPrevouts (unless ANYONECANPAY)
    // 3. hashSequence (unless ANYONECANPAY, SINGLE, NONE)
    // 4. outpoint
    // 5. scriptCode
    // 6. amount
    // 7. nSequence
    // 8. hashOutputs
    // 9. nLocktime
    // 10. sighash type
    
    // For now, return a placeholder - real implementation needs SHA256
    // This is a simplified version for debugging purposes
    
    (void)tx;
    (void)inputIndex;
    (void)scriptCode;
    (void)amount;
    (void)baseType;
    (void)anyoneCanPay;
    
    // TODO: Implement proper BIP143 sighash with SHA256
    
    return result;
}

} // namespace SigHash

// TxBuilder implementation

TxBuilder::TxBuilder() {
    tx_.nVersion = 2;
    tx_.nLockTime = 0;
}

TxBuilder& TxBuilder::SetVersion(int32_t version) {
    tx_.nVersion = version;
    return *this;
}

TxBuilder& TxBuilder::SetLockTime(uint32_t lockTime) {
    tx_.nLockTime = lockTime;
    return *this;
}

TxBuilder& TxBuilder::AddInput(
    const std::string& txid,
    uint32_t vout,
    const CRxdScript& scriptSig,
    uint32_t sequence)
{
    CRxdTxIn in;
    
    // Parse txid from hex (and reverse for internal representation)
    auto txidBytes = FromHex(txid);
    if (txidBytes.size() != 32) {
        throw std::runtime_error("Invalid txid length");
    }
    std::reverse(txidBytes.begin(), txidBytes.end());
    in.prevout.txid = txidBytes;
    in.prevout.n = vout;
    in.scriptSig = scriptSig;
    in.nSequence = sequence;
    
    tx_.vin.push_back(in);
    return *this;
}

TxBuilder& TxBuilder::AddOutput(int64_t value, const CRxdScript& scriptPubKey) {
    CRxdTxOut out;
    out.nValue = value;
    out.scriptPubKey = scriptPubKey;
    tx_.vout.push_back(out);
    return *this;
}

CRxdTx TxBuilder::Build() const {
    return tx_.ToTx();
}

} // namespace rxd
