// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_TX_H
#define RXD_TX_H

#include "rxd_script.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace rxd {

/**
 * Outpoint - reference to a specific output of a transaction
 */
struct CRxdOutPoint {
    std::vector<uint8_t> txid;  // 32 bytes, little-endian
    uint32_t n;                 // output index
    
    CRxdOutPoint() : txid(32, 0), n(0xffffffff) {}
    CRxdOutPoint(const std::vector<uint8_t>& txid_, uint32_t n_) : txid(txid_), n(n_) {}
    
    bool IsNull() const { return n == 0xffffffff; }
    
    std::string ToString() const;
    std::string ToHex() const;
    
    /**
     * Create 36-byte reference (for OP_PUSHINPUTREF)
     */
    std::vector<uint8_t> ToRef() const;
    
    static CRxdOutPoint FromRef(const std::vector<uint8_t>& ref);
};

/**
 * Transaction input
 */
struct CRxdTxIn {
    CRxdOutPoint prevout;
    CRxdScript scriptSig;
    uint32_t nSequence;
    
    static constexpr uint32_t SEQUENCE_FINAL = 0xffffffff;
    static constexpr uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);
    static constexpr uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);
    static constexpr uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;
    
    CRxdTxIn() : nSequence(SEQUENCE_FINAL) {}
    CRxdTxIn(const CRxdOutPoint& prevout_, const CRxdScript& scriptSig_, uint32_t seq = SEQUENCE_FINAL)
        : prevout(prevout_), scriptSig(scriptSig_), nSequence(seq) {}
    
    bool IsFinal() const { return nSequence == SEQUENCE_FINAL; }
    
    // Accessors for compatibility
    std::vector<uint8_t> GetPrevTxId() const { return prevout.txid; }
    uint32_t GetPrevIndex() const { return prevout.n; }
    uint32_t GetSequence() const { return nSequence; }
    const CRxdScript& GetScript() const { return scriptSig; }
    
    // Setters for compatibility
    void SetPrevTxId(const std::vector<uint8_t>& txid) { prevout.txid = txid; }
    void SetPrevIndex(uint32_t n) { prevout.n = n; }
    void SetSequence(uint32_t seq) { nSequence = seq; }
    void SetScript(const CRxdScript& script) { scriptSig = script; }
    
    std::string ToString() const;
};

/**
 * Transaction output
 */
struct CRxdTxOut {
    int64_t nValue;             // in photons (satoshis)
    CRxdScript scriptPubKey;
    
    CRxdTxOut() : nValue(-1) {}
    CRxdTxOut(int64_t value, const CRxdScript& script) : nValue(value), scriptPubKey(script) {}
    
    bool IsNull() const { return nValue < 0; }
    
    // Accessors for compatibility
    int64_t GetValue() const { return nValue; }
    const CRxdScript& GetScript() const { return scriptPubKey; }
    
    // Setters for compatibility
    void SetValue(int64_t value) { nValue = value; }
    void SetScript(const CRxdScript& script) { scriptPubKey = script; }
    
    std::string ToString() const;
};

/**
 * CRxdTx - Radiant transaction
 * 
 * Mirrors btcdeb's CTransaction but with Radiant-specific serialization.
 */
class CRxdTx {
public:
    // Transaction version
    int32_t nVersion;
    
    // Inputs
    std::vector<CRxdTxIn> vin;
    
    // Outputs
    std::vector<CRxdTxOut> vout;
    
    // Locktime
    uint32_t nLockTime;
    
    CRxdTx() : nVersion(2), nLockTime(0) {}
    
    // Accessors
    
    bool IsNull() const { return vin.empty() && vout.empty(); }
    
    // Getters for compatibility
    const std::vector<CRxdTxIn>& GetInputs() const { return vin; }
    const std::vector<CRxdTxOut>& GetOutputs() const { return vout; }
    int32_t GetVersion() const { return nVersion; }
    uint32_t GetLockTime() const { return nLockTime; }
    
    // Setters for compatibility
    void SetVersion(int32_t version) { nVersion = version; }
    void SetLockTime(uint32_t locktime) { nLockTime = locktime; }
    void AddInput(const CRxdTxIn& input) { vin.push_back(input); }
    void AddOutput(const CRxdTxOut& output) { vout.push_back(output); }
    
    /**
     * Get transaction hash (txid)
     */
    std::vector<uint8_t> GetHash() const;
    
    /**
     * Get transaction hash as hex string
     */
    std::string GetHashHex() const;
    
    /**
     * Get total input count
     */
    size_t GetInputCount() const { return vin.size(); }
    
    /**
     * Get total output count
     */
    size_t GetOutputCount() const { return vout.size(); }
    
    /**
     * Get total output value
     */
    int64_t GetValueOut() const;
    
    /**
     * Check if transaction is coinbase
     */
    bool IsCoinBase() const;
    
    // Serialization
    
    /**
     * Serialize to bytes
     */
    std::vector<uint8_t> Serialize() const;
    
    /**
     * Serialize to hex string
     */
    std::string ToHex() const;
    
    /**
     * Parse from hex string
     */
    static CRxdTx FromHex(const std::string& hex);
    
    /**
     * Parse from bytes
     */
    static CRxdTx Deserialize(const std::vector<uint8_t>& data);
    static CRxdTx Deserialize(const uint8_t* data, size_t len);
    
    // Debug display
    
    std::string ToString() const;
};

/**
 * Mutable transaction (for building)
 */
class CMutableRxdTx {
public:
    int32_t nVersion;
    std::vector<CRxdTxIn> vin;
    std::vector<CRxdTxOut> vout;
    uint32_t nLockTime;
    
    CMutableRxdTx() : nVersion(2), nLockTime(0) {}
    explicit CMutableRxdTx(const CRxdTx& tx);
    
    /**
     * Convert to immutable transaction
     */
    CRxdTx ToTx() const;
    
    /**
     * Get hash (computed fresh each time)
     */
    std::vector<uint8_t> GetHash() const;
};

/**
 * Signature hash computation
 */
namespace SigHash {
    /**
     * Compute signature hash for input
     * 
     * @param tx          Transaction being signed
     * @param inputIndex  Index of input being signed
     * @param scriptCode  Script being executed (usually scriptPubKey)
     * @param amount      Amount of UTXO being spent
     * @param sigHashType Signature hash type flags
     * @return 32-byte signature hash
     */
    std::vector<uint8_t> ComputeSigHash(
        const CRxdTx& tx,
        unsigned int inputIndex,
        const CRxdScript& scriptCode,
        int64_t amount,
        uint32_t sigHashType
    );
}

/**
 * Transaction builder helper
 */
class TxBuilder {
public:
    TxBuilder();
    
    TxBuilder& SetVersion(int32_t version);
    TxBuilder& SetLockTime(uint32_t lockTime);
    
    TxBuilder& AddInput(
        const std::string& txid,
        uint32_t vout,
        const CRxdScript& scriptSig = CRxdScript(),
        uint32_t sequence = CRxdTxIn::SEQUENCE_FINAL
    );
    
    TxBuilder& AddOutput(int64_t value, const CRxdScript& scriptPubKey);
    
    CRxdTx Build() const;
    
private:
    CMutableRxdTx tx_;
};

} // namespace rxd

#endif // RXD_TX_H
