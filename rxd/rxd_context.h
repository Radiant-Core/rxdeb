// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_CONTEXT_H
#define RXD_CONTEXT_H

#include "rxd_script.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <set>

namespace rxd {

// Forward declarations
class CRxdTx;

/**
 * Coin/UTXO being spent
 */
struct Coin {
    int64_t value;              // in photons
    CRxdScript scriptPubKey;
    uint32_t height;            // block height (0 if unconfirmed)
    bool isCoinbase;
    
    Coin() : value(0), height(0), isCoinbase(false) {}
    Coin(int64_t val, const CRxdScript& script, uint32_t h = 0, bool cb = false)
        : value(val), scriptPubKey(script), height(h), isCoinbase(cb) {}
};

/**
 * Reference (36 bytes: 32-byte txid + 4-byte vout)
 */
using RefType = std::vector<uint8_t>;

/**
 * Summary of push references in a script
 */
struct PushRefScriptSummary {
    int64_t value;
    std::set<RefType> pushRefSet;
    std::set<RefType> requireRefSet;
    std::set<RefType> disallowSiblingRefSet;
    std::set<RefType> singletonRefSet;
    std::vector<uint8_t> codeScriptHash;
    uint32_t stateSeparatorByteIndex;
};

/**
 * RxdExecutionContext - Full execution context for introspection opcodes
 * 
 * This class holds all the data needed for native introspection opcodes
 * to query transaction state, input UTXOs, and reference tracking.
 * 
 * Reference: Radiant-Core src/script/script_execution_context.h
 */
class RxdExecutionContext {
public:
    /**
     * Construct execution context
     * 
     * @param tx          Transaction being validated
     * @param inputCoins  All input coins (UTXOs being spent)
     * @param inputIndex  Index of input being validated
     */
    RxdExecutionContext(
        std::shared_ptr<const CRxdTx> tx,
        std::vector<Coin> inputCoins,
        unsigned int inputIndex
    );
    
    ~RxdExecutionContext();
    
    // Transaction accessors
    
    /**
     * Get the transaction being validated
     */
    const CRxdTx& GetTx() const;
    
    /**
     * Get current input index
     */
    unsigned int GetInputIndex() const;
    
    /**
     * Get number of inputs
     */
    size_t GetInputCount() const;
    
    /**
     * Get number of outputs
     */
    size_t GetOutputCount() const;
    
    /**
     * Get transaction version
     */
    int32_t GetTxVersion() const;
    
    /**
     * Get transaction locktime
     */
    uint32_t GetLockTime() const;
    
    // Input accessors (for OP_UTXO*, OP_OUTPOINT*, OP_INPUT*)
    
    /**
     * Get coin being spent by input at index
     */
    const Coin& GetInputCoin(unsigned int index) const;
    
    /**
     * Get UTXO value at input index (OP_UTXOVALUE)
     */
    int64_t GetUtxoValue(unsigned int index) const;
    
    /**
     * Get UTXO scriptPubKey at input index (OP_UTXOBYTECODE)
     */
    const CRxdScript& GetUtxoBytecode(unsigned int index) const;
    
    /**
     * Get outpoint txid at input index (OP_OUTPOINTTXHASH)
     */
    std::vector<uint8_t> GetOutpointTxHash(unsigned int index) const;
    
    /**
     * Get outpoint vout at input index (OP_OUTPOINTINDEX)
     */
    uint32_t GetOutpointIndex(unsigned int index) const;
    
    /**
     * Get input scriptSig at index (OP_INPUTBYTECODE)
     */
    CRxdScript GetInputBytecode(unsigned int index) const;
    
    /**
     * Get input sequence number (OP_INPUTSEQUENCENUMBER)
     */
    uint32_t GetInputSequence(unsigned int index) const;
    
    // Output accessors (for OP_OUTPUT*)
    
    /**
     * Get output value at index (OP_OUTPUTVALUE)
     */
    int64_t GetOutputValue(unsigned int index) const;
    
    /**
     * Get output scriptPubKey at index (OP_OUTPUTBYTECODE)
     */
    CRxdScript GetOutputBytecode(unsigned int index) const;
    
    // Active script accessors
    
    /**
     * Get the currently executing script (OP_ACTIVEBYTECODE)
     */
    const CRxdScript& GetActiveBytecode() const;
    
    /**
     * Set the currently executing script
     */
    void SetActiveBytecode(const CRxdScript& script);
    
    // State separator accessors
    
    /**
     * Get state separator index for input UTXO (OP_STATESEPARATORINDEX_UTXO)
     */
    uint32_t GetStateSeparatorIndexUtxo(unsigned int index) const;
    
    /**
     * Get state separator index for output (OP_STATESEPARATORINDEX_OUTPUT)
     */
    uint32_t GetStateSeparatorIndexOutput(unsigned int index) const;
    
    /**
     * Get code script bytecode for UTXO (OP_CODESCRIPTBYTECODE_UTXO)
     */
    CRxdScript GetCodeScriptBytecodeUtxo(unsigned int index) const;
    
    /**
     * Get code script bytecode for output (OP_CODESCRIPTBYTECODE_OUTPUT)
     */
    CRxdScript GetCodeScriptBytecodeOutput(unsigned int index) const;
    
    /**
     * Get state script bytecode for UTXO (OP_STATESCRIPTBYTECODE_UTXO)
     */
    CRxdScript GetStateScriptBytecodeUtxo(unsigned int index) const;
    
    /**
     * Get state script bytecode for output (OP_STATESCRIPTBYTECODE_OUTPUT)
     */
    CRxdScript GetStateScriptBytecodeOutput(unsigned int index) const;
    
    // Reference tracking accessors
    
    /**
     * Get push ref summary for input UTXO
     */
    const PushRefScriptSummary& GetInputPushRefSummary(unsigned int index) const;
    
    /**
     * Get push ref summary for output
     */
    const PushRefScriptSummary& GetOutputPushRefSummary(unsigned int index) const;
    
    /**
     * Get all push refs from inputs
     */
    const std::set<RefType>& GetInputPushRefs() const;
    
    /**
     * Get all push refs in outputs
     */
    const std::set<RefType>& GetOutputPushRefs() const;
    
    // Reference value/count queries
    
    /**
     * Get sum of values for a reference across inputs (OP_REFVALUESUM_UTXOS)
     */
    int64_t GetRefValueSumUtxos(const RefType& ref) const;
    
    /**
     * Get sum of values for a reference across outputs (OP_REFVALUESUM_OUTPUTS)
     */
    int64_t GetRefValueSumOutputs(const RefType& ref) const;
    
    /**
     * Get count of outputs with reference in inputs (OP_REFOUTPUTCOUNT_UTXOS)
     */
    uint32_t GetRefOutputCountUtxos(const RefType& ref) const;
    
    /**
     * Get count of outputs with reference (OP_REFOUTPUTCOUNT_OUTPUTS)
     */
    uint32_t GetRefOutputCountOutputs(const RefType& ref) const;
    
    // Code script hash queries
    
    /**
     * Get sum of values for code script hash in inputs (OP_CODESCRIPTHASHVALUESUM_UTXOS)
     */
    int64_t GetCodeScriptHashValueSumUtxos(const std::vector<uint8_t>& csh) const;
    
    /**
     * Get sum of values for code script hash in outputs (OP_CODESCRIPTHASHVALUESUM_OUTPUTS)
     */
    int64_t GetCodeScriptHashValueSumOutputs(const std::vector<uint8_t>& csh) const;
    
    /**
     * Get count of outputs with code script hash in inputs (OP_CODESCRIPTHASHOUTPUTCOUNT_UTXOS)
     */
    uint32_t GetCodeScriptHashOutputCountUtxos(const std::vector<uint8_t>& csh) const;
    
    /**
     * Get count of outputs with code script hash (OP_CODESCRIPTHASHOUTPUTCOUNT_OUTPUTS)
     */
    uint32_t GetCodeScriptHashOutputCountOutputs(const std::vector<uint8_t>& csh) const;
    
    // Validation helpers
    
    /**
     * Check if context is valid for introspection
     */
    bool IsValid() const;
    
    /**
     * Check if input index is valid
     */
    bool IsValidInputIndex(unsigned int index) const;
    
    /**
     * Check if output index is valid
     */
    bool IsValidOutputIndex(unsigned int index) const;
    
    // Debug display
    
    /**
     * Get human-readable context summary
     */
    std::string ToString() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Create a minimal execution context for simple script testing
 * (without full transaction context)
 */
std::shared_ptr<RxdExecutionContext> CreateMinimalContext();

/**
 * Create execution context from transaction and input coins
 */
std::shared_ptr<RxdExecutionContext> CreateContext(
    std::shared_ptr<const CRxdTx> tx,
    const std::vector<Coin>& inputCoins,
    unsigned int inputIndex
);

} // namespace rxd

#endif // RXD_CONTEXT_H
