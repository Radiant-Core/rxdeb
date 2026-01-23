// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_core_bridge.h"
#include "rxd_vm_adapter.h"
#include "rxd_signature.h"
#include "rxd_context.h"
#include "rxd_tx.h"

namespace rxd {
namespace core {

const char* ScriptErrorString(ScriptError err) {
    switch (err) {
        case SCRIPT_ERR_OK: return "No error";
        case SCRIPT_ERR_UNKNOWN_ERROR: return "Unknown error";
        case SCRIPT_ERR_EVAL_FALSE: return "Script evaluated without error but finished with a false/empty top stack element";
        case SCRIPT_ERR_OP_RETURN: return "OP_RETURN was encountered";
        case SCRIPT_ERR_SCRIPT_SIZE: return "Script is too big";
        case SCRIPT_ERR_PUSH_SIZE: return "Push value size limit exceeded";
        case SCRIPT_ERR_OP_COUNT: return "Operation limit exceeded";
        case SCRIPT_ERR_STACK_SIZE: return "Stack size limit exceeded";
        case SCRIPT_ERR_SIG_COUNT: return "Signature count negative or greater than pubkey count";
        case SCRIPT_ERR_PUBKEY_COUNT: return "Pubkey count negative or limit exceeded";
        case SCRIPT_ERR_VERIFY: return "Script failed an OP_VERIFY operation";
        case SCRIPT_ERR_EQUALVERIFY: return "Script failed an OP_EQUALVERIFY operation";
        case SCRIPT_ERR_CHECKMULTISIGVERIFY: return "Script failed an OP_CHECKMULTISIGVERIFY operation";
        case SCRIPT_ERR_CHECKSIGVERIFY: return "Script failed an OP_CHECKSIGVERIFY operation";
        case SCRIPT_ERR_NUMEQUALVERIFY: return "Script failed an OP_NUMEQUALVERIFY operation";
        case SCRIPT_ERR_BAD_OPCODE: return "Opcode missing or not understood";
        case SCRIPT_ERR_DISABLED_OPCODE: return "Attempted to use a disabled opcode";
        case SCRIPT_ERR_INVALID_STACK_OPERATION: return "Operation not valid with the current stack size";
        case SCRIPT_ERR_INVALID_ALTSTACK_OPERATION: return "Operation not valid with the current altstack size";
        case SCRIPT_ERR_UNBALANCED_CONDITIONAL: return "Invalid OP_IF construction";
        case SCRIPT_ERR_SIG_HASHTYPE: return "Signature hash type missing or not understood";
        case SCRIPT_ERR_SIG_DER: return "Non-canonical DER signature";
        case SCRIPT_ERR_MINIMALDATA: return "Data push larger than necessary";
        case SCRIPT_ERR_SIG_PUSHONLY: return "Only push operators allowed in signatures";
        case SCRIPT_ERR_SIG_HIGH_S: return "Non-canonical signature: S value is unnecessarily high";
        case SCRIPT_ERR_SIG_NULLDUMMY: return "Dummy CHECKMULTISIG argument must be zero";
        case SCRIPT_ERR_PUBKEYTYPE: return "Public key is neither compressed or uncompressed";
        case SCRIPT_ERR_CLEANSTACK: return "Stack size must be exactly one after execution";
        case SCRIPT_ERR_MINIMALIF: return "OP_IF/NOTIF argument must be minimal";
        case SCRIPT_ERR_SIG_NULLFAIL: return "Signature must be zero for failed CHECK(MULTI)SIG operation";
        case SCRIPT_ERR_NEGATIVE_LOCKTIME: return "Negative locktime";
        case SCRIPT_ERR_UNSATISFIED_LOCKTIME: return "Locktime requirement not satisfied";
        case SCRIPT_ERR_DIV_BY_ZERO: return "Division by zero";
        case SCRIPT_ERR_MOD_BY_ZERO: return "Modulo by zero";
        case SCRIPT_ERR_INVALID_NUMBER_RANGE: return "Number out of range";
        case SCRIPT_ERR_IMPOSSIBLE_ENCODING: return "The requested encoding is impossible to satisfy";
        case SCRIPT_ERR_CONTEXT_NOT_PRESENT: return "Execution context not present for introspection";
        case SCRIPT_ERR_INVALID_TX_INPUT_INDEX: return "Invalid transaction input index for introspection";
        case SCRIPT_ERR_INVALID_TX_OUTPUT_INDEX: return "Invalid transaction output index for introspection";
        case SCRIPT_ERR_INVALID_STATE_SEPARATOR_LOCATION: return "State separator in invalid location";
        case SCRIPT_ERR_MUST_USE_FORKID: return "Signature must use SIGHASH_FORKID";
        default: return "Unknown error";
    }
}

#ifdef USE_RADIANT_CORE

// When compiled with Radiant-Core support, use their interpreter

#include "deps/radiant-core/src/script/interpreter.h"
#include "deps/radiant-core/src/script/script.h"

static bool g_radiantCoreInitialized = false;

bool IsRadiantCoreAvailable() {
    return g_radiantCoreInitialized;
}

std::string GetRadiantCoreVersion() {
    // TODO: Get actual version from Radiant-Core
    return "Radiant-Core (integrated)";
}

bool InitRadiantCore() {
    // Initialize Radiant-Core subsystems as needed
    g_radiantCoreInitialized = true;
    return true;
}

void ShutdownRadiantCore() {
    g_radiantCoreInitialized = false;
}

VerifyResult VerifyScript(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    const CRxdTx& tx,
    unsigned int nIn,
    int64_t amount,
    uint32_t flags)
{
    VerifyResult result;
    
    if (!g_radiantCoreInitialized) {
        // Fall back to native implementation
        goto native_verify;
    }
    
    // TODO: Convert rxd types to Radiant-Core types and call their VerifyScript
    // This requires proper type bridging between the two codebases
    
native_verify:
    // Native verification using rxdeb's VM
    auto ctx = CreateMinimalContext();
    RxdVMAdapter vm(scriptSig, scriptPubKey, tx, nIn, amount, ctx);
    
    result.success = vm.Run();
    if (!result.success) {
        result.error = SCRIPT_ERR_EVAL_FALSE;
        result.errorMessage = vm.GetErrorString();
    } else {
        result.error = SCRIPT_ERR_OK;
    }
    
    auto state = vm.GetState();
    result.opCount = static_cast<int>(state.opCount);
    result.stackSize = state.stack.size();
    
    return result;
}

#else // !USE_RADIANT_CORE

bool IsRadiantCoreAvailable() {
    return false;
}

std::string GetRadiantCoreVersion() {
    return "Radiant-Core not available (native mode)";
}

VerifyResult VerifyScript(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    const CRxdTx& tx,
    unsigned int nIn,
    int64_t amount,
    uint32_t flags)
{
    VerifyResult result;
    
    // Build input coins from transaction
    std::vector<Coin> inputCoins;
    for (size_t i = 0; i < tx.GetInputs().size(); i++) {
        Coin coin;
        // For the input being verified, use the provided amount and scriptPubKey
        if (i == nIn) {
            coin.value = amount;
            coin.scriptPubKey = scriptPubKey;
        }
        inputCoins.push_back(coin);
    }
    
    // Create execution context
    auto txPtr = std::make_shared<CRxdTx>(tx);
    auto ctx = CreateContext(txPtr, inputCoins, nIn);
    
    // Create VM and run
    RxdVMAdapter vm(scriptSig, scriptPubKey, tx, nIn, amount, ctx);
    
    result.success = vm.Run();
    if (!result.success) {
        result.error = SCRIPT_ERR_EVAL_FALSE;
        result.errorMessage = vm.GetErrorString();
        
        // Try to map specific errors
        std::string err = result.errorMessage;
        if (err.find("stack") != std::string::npos) {
            result.error = SCRIPT_ERR_INVALID_STACK_OPERATION;
        } else if (err.find("OP_RETURN") != std::string::npos) {
            result.error = SCRIPT_ERR_OP_RETURN;
        } else if (err.find("VERIFY") != std::string::npos) {
            result.error = SCRIPT_ERR_VERIFY;
        } else if (err.find("division") != std::string::npos || 
                   err.find("zero") != std::string::npos) {
            result.error = SCRIPT_ERR_DIV_BY_ZERO;
        }
    } else {
        result.error = SCRIPT_ERR_OK;
    }
    
    auto state = vm.GetState();
    result.opCount = static_cast<int>(state.opCount);
    result.stackSize = state.stack.size();
    
    // Check clean stack if required
    if (result.success && (flags & SCRIPT_VERIFY_CLEANSTACK)) {
        if (state.stack.size() != 1) {
            result.success = false;
            result.error = SCRIPT_ERR_CLEANSTACK;
            result.errorMessage = "Stack size must be exactly one after execution";
        }
    }
    
    return result;
}

#endif // USE_RADIANT_CORE

std::vector<VerifyResult> VerifyTransaction(
    const CRxdTx& tx,
    const std::vector<std::pair<CRxdScript, int64_t>>& utxos,
    uint32_t flags)
{
    std::vector<VerifyResult> results;
    
    if (tx.GetInputs().size() != utxos.size()) {
        VerifyResult err;
        err.success = false;
        err.error = SCRIPT_ERR_UNKNOWN_ERROR;
        err.errorMessage = "UTXO count mismatch";
        results.push_back(err);
        return results;
    }
    
    for (size_t i = 0; i < tx.GetInputs().size(); i++) {
        const auto& input = tx.GetInputs()[i];
        const auto& [scriptPubKey, amount] = utxos[i];
        
        auto result = VerifyScript(
            input.GetScript(),
            scriptPubKey,
            tx,
            static_cast<unsigned int>(i),
            amount,
            flags);
        
        results.push_back(result);
    }
    
    return results;
}

} // namespace core
} // namespace rxd
