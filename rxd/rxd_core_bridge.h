// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_CORE_BRIDGE_H
#define RXD_CORE_BRIDGE_H

/**
 * Bridge layer for integrating with Radiant-Core
 * 
 * This module provides an interface to use Radiant-Core's consensus-accurate
 * script interpreter when available. When Radiant-Core is not compiled in,
 * it falls back to rxdeb's native implementation.
 * 
 * Build with -DUSE_RADIANT_CORE to enable Radiant-Core integration.
 */

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#include "rxd_script.h"
#include "rxd_tx.h"
#include "rxd_context.h"

namespace rxd {
namespace core {

/**
 * Check if Radiant-Core integration is available
 */
bool IsRadiantCoreAvailable();

/**
 * Get Radiant-Core version string
 */
std::string GetRadiantCoreVersion();

/**
 * Script verification flags (mirrors Radiant-Core's script flags)
 */
enum ScriptVerifyFlags : uint32_t {
    SCRIPT_VERIFY_NONE                 = 0,
    SCRIPT_VERIFY_P2SH                 = (1U << 0),
    SCRIPT_VERIFY_STRICTENC            = (1U << 1),
    SCRIPT_VERIFY_DERSIG               = (1U << 2),
    SCRIPT_VERIFY_LOW_S                = (1U << 3),
    SCRIPT_VERIFY_NULLDUMMY            = (1U << 4),
    SCRIPT_VERIFY_SIGPUSHONLY          = (1U << 5),
    SCRIPT_VERIFY_MINIMALDATA          = (1U << 6),
    SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS = (1U << 7),
    SCRIPT_VERIFY_CLEANSTACK           = (1U << 8),
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY  = (1U << 9),
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY  = (1U << 10),
    SCRIPT_VERIFY_MINIMALIF            = (1U << 13),
    SCRIPT_VERIFY_NULLFAIL             = (1U << 14),
    SCRIPT_VERIFY_COMPRESSED_PUBKEYTYPE = (1U << 15),
    SCRIPT_VERIFY_SIGHASH_FORKID       = (1U << 16),
    SCRIPT_ENABLE_SIGHASH_FORKID       = (1U << 16),
    SCRIPT_ENABLE_REPLAY_PROTECTION    = (1U << 17),
    SCRIPT_ENABLE_CHECKDATASIG         = (1U << 18),
    SCRIPT_ENABLE_SCHNORR              = (1U << 19),
    SCRIPT_ENABLE_OP_REVERSEBYTES      = (1U << 20),
    SCRIPT_ENABLE_NATIVE_INTROSPECTION = (1U << 21),
    SCRIPT_64_BIT_INTEGERS             = (1U << 22),
    SCRIPT_ENABLE_MUL                  = (1U << 23),
    SCRIPT_ENABLE_INDUCTION_OPCODES    = (1U << 24),
    
    // Radiant standard flags
    SCRIPT_VERIFY_RADIANT_STANDARD = 
        SCRIPT_VERIFY_P2SH |
        SCRIPT_VERIFY_STRICTENC |
        SCRIPT_VERIFY_DERSIG |
        SCRIPT_VERIFY_LOW_S |
        SCRIPT_VERIFY_NULLDUMMY |
        SCRIPT_VERIFY_MINIMALDATA |
        SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS |
        SCRIPT_VERIFY_CLEANSTACK |
        SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
        SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
        SCRIPT_VERIFY_MINIMALIF |
        SCRIPT_VERIFY_NULLFAIL |
        SCRIPT_ENABLE_SIGHASH_FORKID |
        SCRIPT_ENABLE_CHECKDATASIG |
        SCRIPT_ENABLE_SCHNORR |
        SCRIPT_ENABLE_OP_REVERSEBYTES |
        SCRIPT_ENABLE_NATIVE_INTROSPECTION |
        SCRIPT_64_BIT_INTEGERS |
        SCRIPT_ENABLE_MUL |
        SCRIPT_ENABLE_INDUCTION_OPCODES
};

/**
 * Script error codes
 */
enum ScriptError {
    SCRIPT_ERR_OK = 0,
    SCRIPT_ERR_UNKNOWN_ERROR,
    SCRIPT_ERR_EVAL_FALSE,
    SCRIPT_ERR_OP_RETURN,
    
    // Stack errors
    SCRIPT_ERR_SCRIPT_SIZE,
    SCRIPT_ERR_PUSH_SIZE,
    SCRIPT_ERR_OP_COUNT,
    SCRIPT_ERR_STACK_SIZE,
    SCRIPT_ERR_SIG_COUNT,
    SCRIPT_ERR_PUBKEY_COUNT,
    
    // Verification errors
    SCRIPT_ERR_VERIFY,
    SCRIPT_ERR_EQUALVERIFY,
    SCRIPT_ERR_CHECKMULTISIGVERIFY,
    SCRIPT_ERR_CHECKSIGVERIFY,
    SCRIPT_ERR_NUMEQUALVERIFY,
    
    // Operational errors
    SCRIPT_ERR_BAD_OPCODE,
    SCRIPT_ERR_DISABLED_OPCODE,
    SCRIPT_ERR_INVALID_STACK_OPERATION,
    SCRIPT_ERR_INVALID_ALTSTACK_OPERATION,
    SCRIPT_ERR_UNBALANCED_CONDITIONAL,
    
    // Signature errors
    SCRIPT_ERR_SIG_HASHTYPE,
    SCRIPT_ERR_SIG_DER,
    SCRIPT_ERR_MINIMALDATA,
    SCRIPT_ERR_SIG_PUSHONLY,
    SCRIPT_ERR_SIG_HIGH_S,
    SCRIPT_ERR_SIG_NULLDUMMY,
    SCRIPT_ERR_PUBKEYTYPE,
    SCRIPT_ERR_CLEANSTACK,
    SCRIPT_ERR_MINIMALIF,
    SCRIPT_ERR_SIG_NULLFAIL,
    
    // Locktime errors
    SCRIPT_ERR_NEGATIVE_LOCKTIME,
    SCRIPT_ERR_UNSATISFIED_LOCKTIME,
    
    // Division errors
    SCRIPT_ERR_DIV_BY_ZERO,
    SCRIPT_ERR_MOD_BY_ZERO,
    
    // Numeric errors
    SCRIPT_ERR_INVALID_NUMBER_RANGE,
    SCRIPT_ERR_IMPOSSIBLE_ENCODING,
    
    // Introspection errors
    SCRIPT_ERR_CONTEXT_NOT_PRESENT,
    SCRIPT_ERR_INVALID_TX_INPUT_INDEX,
    SCRIPT_ERR_INVALID_TX_OUTPUT_INDEX,
    
    // State separator errors  
    SCRIPT_ERR_INVALID_STATE_SEPARATOR_LOCATION,
    
    // Must-use-FORKID
    SCRIPT_ERR_MUST_USE_FORKID,
    
    SCRIPT_ERR_ERROR_COUNT
};

/**
 * Get error string
 */
const char* ScriptErrorString(ScriptError err);

/**
 * Result of script verification
 */
struct VerifyResult {
    bool success;
    ScriptError error;
    std::string errorMessage;
    
    // Execution stats
    int opCount;
    int sigOps;
    size_t stackSize;
    
    VerifyResult() : success(false), error(SCRIPT_ERR_UNKNOWN_ERROR), 
                     opCount(0), sigOps(0), stackSize(0) {}
};

/**
 * Verify a script using Radiant-Core (if available) or native implementation
 * 
 * @param scriptSig Unlocking script
 * @param scriptPubKey Locking script
 * @param tx Transaction being verified
 * @param nIn Input index
 * @param amount Input value in satoshis
 * @param flags Verification flags
 * @return Verification result
 */
VerifyResult VerifyScript(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    const CRxdTx& tx,
    unsigned int nIn,
    int64_t amount,
    uint32_t flags = SCRIPT_VERIFY_RADIANT_STANDARD);

/**
 * Verify a complete transaction
 * 
 * @param tx Transaction to verify
 * @param utxos UTXOs being spent (must match tx inputs)
 * @param flags Verification flags
 * @return Vector of results, one per input
 */
std::vector<VerifyResult> VerifyTransaction(
    const CRxdTx& tx,
    const std::vector<std::pair<CRxdScript, int64_t>>& utxos,
    uint32_t flags = SCRIPT_VERIFY_RADIANT_STANDARD);

#ifdef USE_RADIANT_CORE

/**
 * Initialize Radiant-Core subsystem
 * Must be called before using VerifyScript with Radiant-Core
 */
bool InitRadiantCore();

/**
 * Shutdown Radiant-Core subsystem
 */
void ShutdownRadiantCore();

#endif // USE_RADIANT_CORE

} // namespace core
} // namespace rxd

#endif // RXD_CORE_BRIDGE_H
