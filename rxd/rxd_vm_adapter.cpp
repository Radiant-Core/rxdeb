// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_vm_adapter.h"
#include "rxd_tx.h"
#include "rxd_context.h"
#include "rxd_params.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace rxd {

const char* ScriptErrorString(ScriptError error) {
    switch (error) {
        case ScriptError::OK: return "No error";
        case ScriptError::UNKNOWN: return "Unknown error";
        case ScriptError::EVAL_FALSE: return "Script evaluated without error but finished with a false/empty top stack element";
        case ScriptError::OP_RETURN: return "OP_RETURN was encountered";
        
        case ScriptError::SCRIPT_SIZE: return "Script is too big";
        case ScriptError::PUSH_SIZE: return "Push value size limit exceeded";
        case ScriptError::OP_COUNT: return "Operation limit exceeded";
        case ScriptError::STACK_SIZE: return "Stack size limit exceeded";
        case ScriptError::SIG_COUNT: return "Signature count negative or greater than pubkey count";
        case ScriptError::PUBKEY_COUNT: return "Pubkey count negative or limit exceeded";
        
        case ScriptError::VERIFY: return "Script failed an OP_VERIFY operation";
        case ScriptError::EQUALVERIFY: return "Script failed an OP_EQUALVERIFY operation";
        case ScriptError::CHECKMULTISIGVERIFY: return "Script failed an OP_CHECKMULTISIGVERIFY operation";
        case ScriptError::CHECKSIGVERIFY: return "Script failed an OP_CHECKSIGVERIFY operation";
        case ScriptError::NUMEQUALVERIFY: return "Script failed an OP_NUMEQUALVERIFY operation";
        
        case ScriptError::BAD_OPCODE: return "Opcode missing or not understood";
        case ScriptError::DISABLED_OPCODE: return "Attempted to use a disabled opcode";
        case ScriptError::INVALID_STACK_OPERATION: return "Operation not valid with the current stack size";
        case ScriptError::INVALID_ALTSTACK_OPERATION: return "Operation not valid with the current altstack size";
        case ScriptError::UNBALANCED_CONDITIONAL: return "Invalid OP_IF construction";
        
        case ScriptError::SIG_HASHTYPE: return "Signature hash type missing or not understood";
        case ScriptError::SIG_DER: return "Non-canonical DER signature";
        case ScriptError::MINIMALDATA: return "Data push larger than necessary";
        case ScriptError::SIG_PUSHONLY: return "Only push operators allowed in signatures";
        case ScriptError::SIG_HIGH_S: return "Non-canonical signature: S value is unnecessarily high";
        case ScriptError::SIG_NULLDUMMY: return "Dummy CHECKMULTISIG argument must be zero";
        case ScriptError::PUBKEYTYPE: return "Public key is neither compressed or uncompressed";
        case ScriptError::CLEANSTACK: return "Stack size must be exactly one after execution";
        case ScriptError::MINIMALIF: return "OP_IF/NOTIF argument must be minimal";
        case ScriptError::SIG_NULLFAIL: return "Signature must be zero for failed CHECK(MULTI)SIG operation";
        
        case ScriptError::NEGATIVE_LOCKTIME: return "Negative locktime";
        case ScriptError::UNSATISFIED_LOCKTIME: return "Locktime requirement not satisfied";
        
        case ScriptError::SIG_BADLENGTH: return "Signature is the wrong length";
        
        case ScriptError::INVALID_REFERENCE: return "Invalid reference format";
        case ScriptError::REFERENCE_NOT_FOUND: return "Required reference not found";
        case ScriptError::SINGLETON_MISMATCH: return "Singleton reference mismatch";
        case ScriptError::INVALID_STATE_SEPARATOR: return "Invalid state separator position";
        case ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE: return "Introspection context not available";
        
        default: return "Unknown error";
    }
}

std::optional<SourceMapEntry> RxdArtifact::GetSourceLocation(size_t pc) const {
    auto it = sourceMap.find(pc);
    if (it != sourceMap.end()) {
        return it->second;
    }
    // Find the nearest previous entry
    for (auto rit = sourceMap.rbegin(); rit != sourceMap.rend(); ++rit) {
        if (rit->first <= pc) {
            return rit->second;
        }
    }
    return std::nullopt;
}

// RxdVMAdapter implementation

struct RxdVMAdapter::Impl {
    CRxdScript scriptSig;
    CRxdScript scriptPubKey;
    std::shared_ptr<const CRxdTx> tx;
    unsigned int inputIndex;
    uint32_t flags;
    std::shared_ptr<RxdExecutionContext> context;
    
    VMState currentState;
    std::vector<VMState> history;
    
    OpcodeCallback opcodeCallback;
    RxdArtifact artifact;
    
    bool initialized = false;
    bool inScriptPubKey = false;  // Track which script phase we're in
    
    Impl(const CRxdScript& sig, const CRxdScript& pubkey,
         const CRxdTx& transaction, unsigned int idx,
         uint32_t f, std::shared_ptr<RxdExecutionContext> ctx)
        : scriptSig(sig)
        , scriptPubKey(pubkey)
        , tx(std::make_shared<CRxdTx>(transaction))
        , inputIndex(idx)
        , flags(f)
        , context(ctx)
    {
        Reset();
    }
    
    void Reset() {
        currentState = VMState{};
        currentState.pc = 0;
        currentState.opIndex = 0;
        currentState.opCount = 0;
        currentState.done = false;
        currentState.success = false;
        currentState.error = ScriptError::OK;
        history.clear();
        initialized = true;
        
        // If scriptSig is empty, start directly with scriptPubKey
        if (scriptSig.empty() && !scriptPubKey.empty()) {
            currentState.script = scriptPubKey;
            inScriptPubKey = true;
        } else {
            currentState.script = scriptSig;
            inScriptPubKey = false;
        }
    }
    
    bool Step() {
        if (currentState.done) {
            return false;
        }
        
        // Save current state to history
        history.push_back(currentState);
        
        VMState stateBefore = currentState;
        
        // Get next opcode
        CRxdScript::const_iterator pc = currentState.script.begin() + currentState.pc;
        if (pc >= currentState.script.end()) {
            // Script exhausted
            if (!inScriptPubKey && scriptPubKey.size() > 0) {
                // Move to scriptPubKey
                currentState.script = scriptPubKey;
                currentState.pc = 0;
                inScriptPubKey = true;
                return true;
            }
            // Execution complete - check for unbalanced conditionals
            currentState.done = true;
            if (!currentState.vfExec.empty()) {
                // Unbalanced IF/ENDIF
                currentState.error = ScriptError::UNBALANCED_CONDITIONAL;
                currentState.success = false;
                return false;
            }
            currentState.success = !currentState.stack.empty() && 
                                   CastToBool(currentState.stack.back());
            if (!currentState.success) {
                currentState.error = ScriptError::EVAL_FALSE;
            }
            return false;
        }
        
        opcodetype opcode;
        valtype pushData;
        
        if (!currentState.script.GetOp(pc, opcode, pushData)) {
            currentState.done = true;
            currentState.error = ScriptError::BAD_OPCODE;
            return false;
        }
        
        size_t newPc = pc - currentState.script.begin();
        
        // Execute the opcode
        ScriptError execError = ExecuteOpcode(opcode, pushData);
        
        if (execError != ScriptError::OK) {
            currentState.done = true;
            currentState.error = execError;
            currentState.success = false;
            return false;
        }
        
        currentState.pc = newPc;
        currentState.opIndex++;
        
        // Call callback if set
        if (opcodeCallback) {
            opcodeCallback(opcode, pushData.empty() ? nullptr : &pushData, 
                          stateBefore, currentState);
        }
        
        return true;
    }
    
    bool Rewind() {
        if (history.empty()) {
            return false;
        }
        currentState = history.back();
        history.pop_back();
        return true;
    }
    
    ScriptError ExecuteOpcode(opcodetype opcode, const valtype& pushData) {
        auto& stack = currentState.stack;
        auto& altstack = currentState.altstack;
        auto& vfExec = currentState.vfExec;
        
        bool fExec = vfExec.empty() || vfExec.back();
        
        // Handle conditional execution
        if (!fExec && (opcode < OP_IF || opcode > OP_ENDIF)) {
            return ScriptError::OK;  // Skip non-conditional opcodes when not executing
        }
        
        // Push data opcodes
        if (opcode <= OP_PUSHDATA4) {
            if (pushData.size() > Limits::MAX_SCRIPT_ELEMENT_SIZE) {
                return ScriptError::PUSH_SIZE;
            }
            stack.push_back(pushData);
            return ScriptError::OK;
        }
        
        // Small integer push
        if (opcode >= OP_1 && opcode <= OP_16) {
            stack.push_back(valtype{static_cast<uint8_t>(opcode - OP_1 + 1)});
            return ScriptError::OK;
        }
        
        if (opcode == OP_1NEGATE) {
            stack.push_back(valtype{0x81});  // -1 in script number format
            return ScriptError::OK;
        }
        
        // Execute based on opcode category
        switch (opcode) {
            // Stack operations
            case OP_DUP:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                stack.push_back(stack.back());
                break;
                
            case OP_DROP:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                stack.pop_back();
                break;
                
            case OP_2DROP:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                stack.pop_back();
                stack.pop_back();
                break;
                
            case OP_2DUP:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype v1 = stack[stack.size() - 2];
                    valtype v2 = stack[stack.size() - 1];
                    stack.push_back(v1);
                    stack.push_back(v2);
                }
                break;
                
            case OP_3DUP:
                if (stack.size() < 3) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype v1 = stack[stack.size() - 3];
                    valtype v2 = stack[stack.size() - 2];
                    valtype v3 = stack[stack.size() - 1];
                    stack.push_back(v1);
                    stack.push_back(v2);
                    stack.push_back(v3);
                }
                break;
                
            case OP_NIP:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                stack.erase(stack.end() - 2);
                break;
                
            case OP_OVER:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                stack.push_back(stack[stack.size() - 2]);
                break;
                
            case OP_SWAP:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
                break;
                
            case OP_ROT:
                if (stack.size() < 3) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype v = stack[stack.size() - 3];
                    stack.erase(stack.end() - 3);
                    stack.push_back(v);
                }
                break;
                
            case OP_TUCK:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                stack.insert(stack.end() - 2, stack.back());
                break;
                
            case OP_DEPTH:
                {
                    int64_t depth = stack.size();
                    stack.push_back(ScriptNumSerialize(depth));
                }
                break;
                
            case OP_TOALTSTACK:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                altstack.push_back(stack.back());
                stack.pop_back();
                break;
                
            case OP_FROMALTSTACK:
                if (altstack.size() < 1) return ScriptError::INVALID_ALTSTACK_OPERATION;
                stack.push_back(altstack.back());
                altstack.pop_back();
                break;
                
            case OP_IFDUP:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                if (CastToBool(stack.back())) {
                    stack.push_back(stack.back());
                }
                break;
                
            case OP_PICK:
            case OP_ROLL:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (n < 0 || static_cast<size_t>(n) >= stack.size())
                        return ScriptError::INVALID_STACK_OPERATION;
                    valtype v = stack[stack.size() - n - 1];
                    if (opcode == OP_ROLL)
                        stack.erase(stack.end() - n - 1);
                    stack.push_back(v);
                }
                break;
            
            // Arithmetic
            case OP_ADD:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a + b));
                }
                break;
                
            case OP_SUB:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a - b));
                }
                break;
                
            case OP_MUL:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a * b));
                }
                break;
                
            case OP_DIV:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    if (b == 0) return ScriptError::INVALID_STACK_OPERATION;
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a / b));
                }
                break;
                
            case OP_MOD:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    if (b == 0) return ScriptError::INVALID_STACK_OPERATION;
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a % b));
                }
                break;
                
            case OP_1ADD:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(n + 1));
                }
                break;
                
            case OP_1SUB:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(n - 1));
                }
                break;
                
            case OP_NEGATE:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(-n));
                }
                break;
                
            case OP_ABS:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(n < 0 ? -n : n));
                }
                break;
                
            case OP_NOT:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(n == 0 ? 1 : 0));
                }
                break;
                
            case OP_0NOTEQUAL:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t n = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(n != 0 ? 1 : 0));
                }
                break;
                
            // Comparison
            case OP_NUMEQUAL:
            case OP_NUMEQUALVERIFY:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    bool result = (a == b);
                    stack.push_back(ScriptNumSerialize(result ? 1 : 0));
                    if (opcode == OP_NUMEQUALVERIFY) {
                        if (!result) return ScriptError::NUMEQUALVERIFY;
                        stack.pop_back();
                    }
                }
                break;
                
            case OP_NUMNOTEQUAL:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a != b ? 1 : 0));
                }
                break;
                
            case OP_LESSTHAN:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a < b ? 1 : 0));
                }
                break;
                
            case OP_GREATERTHAN:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a > b ? 1 : 0));
                }
                break;
                
            case OP_LESSTHANOREQUAL:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a <= b ? 1 : 0));
                }
                break;
                
            case OP_GREATERTHANOREQUAL:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(a >= b ? 1 : 0));
                }
                break;
                
            case OP_MIN:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(std::min(a, b)));
                }
                break;
                
            case OP_MAX:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t a = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t b = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(std::max(a, b)));
                }
                break;
                
            case OP_WITHIN:
                if (stack.size() < 3) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t x = ScriptNumDeserialize(stack[stack.size() - 3]);
                    int64_t min = ScriptNumDeserialize(stack[stack.size() - 2]);
                    int64_t max = ScriptNumDeserialize(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize((min <= x && x < max) ? 1 : 0));
                }
                break;
                
            case OP_BOOLAND:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    bool a = CastToBool(stack[stack.size() - 2]);
                    bool b = CastToBool(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize((a && b) ? 1 : 0));
                }
                break;
                
            case OP_BOOLOR:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    bool a = CastToBool(stack[stack.size() - 2]);
                    bool b = CastToBool(stack[stack.size() - 1]);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize((a || b) ? 1 : 0));
                }
                break;
                
            // Equality
            case OP_EQUAL:
            case OP_EQUALVERIFY:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype& a = stack[stack.size() - 2];
                    valtype& b = stack[stack.size() - 1];
                    bool result = (a == b);
                    stack.pop_back();
                    stack.pop_back();
                    stack.push_back(ScriptNumSerialize(result ? 1 : 0));
                    if (opcode == OP_EQUALVERIFY) {
                        if (!result) return ScriptError::EQUALVERIFY;
                        stack.pop_back();
                    }
                }
                break;
                
            // Size
            case OP_SIZE:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                stack.push_back(ScriptNumSerialize(stack.back().size()));
                break;
                
            // Splice operations (Radiant re-enabled)
            case OP_CAT:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype& a = stack[stack.size() - 2];
                    valtype& b = stack[stack.size() - 1];
                    if (a.size() + b.size() > Limits::MAX_SCRIPT_ELEMENT_SIZE)
                        return ScriptError::PUSH_SIZE;
                    a.insert(a.end(), b.begin(), b.end());
                    stack.pop_back();
                }
                break;
                
            case OP_SPLIT:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype& data = stack[stack.size() - 2];
                    int64_t pos = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (pos < 0 || static_cast<size_t>(pos) > data.size())
                        return ScriptError::INVALID_STACK_OPERATION;
                    valtype left(data.begin(), data.begin() + pos);
                    valtype right(data.begin() + pos, data.end());
                    stack.pop_back();
                    stack.push_back(left);
                    stack.push_back(right);
                }
                break;
                
            // Bitwise
            case OP_AND:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype& a = stack[stack.size() - 2];
                    valtype& b = stack[stack.size() - 1];
                    if (a.size() != b.size())
                        return ScriptError::INVALID_STACK_OPERATION;
                    for (size_t i = 0; i < a.size(); i++)
                        a[i] &= b[i];
                    stack.pop_back();
                }
                break;
                
            case OP_OR:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype& a = stack[stack.size() - 2];
                    valtype& b = stack[stack.size() - 1];
                    if (a.size() != b.size())
                        return ScriptError::INVALID_STACK_OPERATION;
                    for (size_t i = 0; i < a.size(); i++)
                        a[i] |= b[i];
                    stack.pop_back();
                }
                break;
                
            case OP_XOR:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype& a = stack[stack.size() - 2];
                    valtype& b = stack[stack.size() - 1];
                    if (a.size() != b.size())
                        return ScriptError::INVALID_STACK_OPERATION;
                    for (size_t i = 0; i < a.size(); i++)
                        a[i] ^= b[i];
                    stack.pop_back();
                }
                break;
                
            // Control flow
            case OP_IF:
            case OP_NOTIF:
                {
                    bool fValue = false;
                    if (fExec) {
                        if (stack.size() < 1)
                            return ScriptError::INVALID_STACK_OPERATION;
                        fValue = CastToBool(stack.back());
                        if (opcode == OP_NOTIF)
                            fValue = !fValue;
                        stack.pop_back();
                    }
                    vfExec.push_back(fValue);
                }
                break;
                
            case OP_ELSE:
                if (vfExec.empty())
                    return ScriptError::UNBALANCED_CONDITIONAL;
                vfExec.back() = !vfExec.back();
                break;
                
            case OP_ENDIF:
                if (vfExec.empty())
                    return ScriptError::UNBALANCED_CONDITIONAL;
                vfExec.pop_back();
                break;
                
            case OP_VERIFY:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                if (!CastToBool(stack.back()))
                    return ScriptError::VERIFY;
                stack.pop_back();
                break;
                
            case OP_RETURN:
                return ScriptError::OP_RETURN;
                
            case OP_NOP:
            case OP_NOP1:
            case OP_NOP4:
            case OP_NOP5:
            case OP_NOP6:
            case OP_NOP7:
            case OP_NOP8:
            case OP_NOP9:
            case OP_NOP10:
                // Do nothing
                break;
                
            // Radiant: Native Introspection opcodes
            case OP_INPUTINDEX:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                stack.push_back(ScriptNumSerialize(context->GetInputIndex()));
                break;
                
            case OP_TXVERSION:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                stack.push_back(ScriptNumSerialize(context->GetTxVersion()));
                break;
                
            case OP_TXINPUTCOUNT:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                stack.push_back(ScriptNumSerialize(context->GetInputCount()));
                break;
                
            case OP_TXOUTPUTCOUNT:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                stack.push_back(ScriptNumSerialize(context->GetOutputCount()));
                break;
                
            case OP_TXLOCKTIME:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                stack.push_back(ScriptNumSerialize(context->GetLockTime()));
                break;
                
            case OP_UTXOVALUE:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t idx = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (!context->IsValidInputIndex(idx))
                        return ScriptError::INVALID_STACK_OPERATION;
                    stack.push_back(ScriptNumSerialize(context->GetUtxoValue(idx)));
                }
                break;
                
            case OP_UTXOBYTECODE:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t idx = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (!context->IsValidInputIndex(idx))
                        return ScriptError::INVALID_STACK_OPERATION;
                    const auto& script = context->GetUtxoBytecode(idx);
                    stack.push_back(valtype(script.begin(), script.end()));
                }
                break;
                
            case OP_OUTPUTVALUE:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t idx = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (!context->IsValidOutputIndex(idx))
                        return ScriptError::INVALID_STACK_OPERATION;
                    stack.push_back(ScriptNumSerialize(context->GetOutputValue(idx)));
                }
                break;
                
            case OP_OUTPUTBYTECODE:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t idx = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (!context->IsValidOutputIndex(idx))
                        return ScriptError::INVALID_STACK_OPERATION;
                    auto script = context->GetOutputBytecode(idx);
                    stack.push_back(valtype(script.begin(), script.end()));
                }
                break;
                
            case OP_INPUTSEQUENCENUMBER:
                if (!context) return ScriptError::INTROSPECTION_CONTEXT_UNAVAILABLE;
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    int64_t idx = ScriptNumDeserialize(stack.back());
                    stack.pop_back();
                    if (!context->IsValidInputIndex(idx))
                        return ScriptError::INVALID_STACK_OPERATION;
                    stack.push_back(ScriptNumSerialize(context->GetInputSequence(idx)));
                }
                break;
                
            // Radiant: State separator
            case OP_STATESEPARATOR:
                // State separator is a no-op during execution but marks
                // the boundary between state and code script
                break;
                
            // Reference opcodes - stub implementations
            case OP_PUSHINPUTREF:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    // The reference is in the next 36 bytes of script
                    // For now, just track it
                    valtype ref = stack.back();
                    if (ref.size() != 36)
                        return ScriptError::INVALID_REFERENCE;
                    currentState.pushRefs.insert(ref);
                }
                break;
                
            case OP_REQUIREINPUTREF:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                {
                    valtype ref = stack.back();
                    stack.pop_back();
                    if (ref.size() != 36)
                        return ScriptError::INVALID_REFERENCE;
                    currentState.requireRefs.insert(ref);
                    // Validation would check if ref exists in inputs
                }
                break;
                
            // Hash opcodes
            case OP_SHA256:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                // TODO: Implement SHA256
                break;
                
            case OP_HASH160:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                // TODO: Implement HASH160 (SHA256 + RIPEMD160)
                break;
                
            case OP_HASH256:
                if (stack.size() < 1) return ScriptError::INVALID_STACK_OPERATION;
                // TODO: Implement HASH256 (double SHA256)
                break;
                
            // Signature opcodes - require transaction context
            case OP_CHECKSIG:
            case OP_CHECKSIGVERIFY:
                if (stack.size() < 2) return ScriptError::INVALID_STACK_OPERATION;
                {
                    // TODO: Implement actual signature verification
                    // For now, just pop the values and push true
                    stack.pop_back();  // pubkey
                    stack.pop_back();  // signature
                    stack.push_back(ScriptNumSerialize(1));  // assume valid for stepping
                    if (opcode == OP_CHECKSIGVERIFY) {
                        stack.pop_back();
                    }
                }
                break;
                
            default:
                // Unknown or unimplemented opcode
                return ScriptError::BAD_OPCODE;
        }
        
        // Check stack size limit
        if (stack.size() + altstack.size() > Limits::MAX_STACK_SIZE) {
            return ScriptError::STACK_SIZE;
        }
        
        return ScriptError::OK;
    }
    
    // Helper functions
    static bool CastToBool(const valtype& v) {
        for (size_t i = 0; i < v.size(); i++) {
            if (v[i] != 0) {
                // Can be negative zero
                if (i == v.size() - 1 && v[i] == 0x80)
                    return false;
                return true;
            }
        }
        return false;
    }
    
    static int64_t ScriptNumDeserialize(const valtype& v) {
        if (v.empty()) return 0;
        
        int64_t result = 0;
        for (size_t i = 0; i < v.size(); i++) {
            result |= static_cast<int64_t>(v[i]) << (8 * i);
        }
        
        // If the input's most significant byte is 0x80, remove it from
        // the result and set the sign bit
        if (v.back() & 0x80) {
            result &= ~(0x80LL << (8 * (v.size() - 1)));
            result = -result;
        }
        
        return result;
    }
    
    static valtype ScriptNumSerialize(int64_t n) {
        if (n == 0) return valtype{};
        
        valtype result;
        bool neg = n < 0;
        uint64_t absvalue = neg ? -n : n;
        
        while (absvalue) {
            result.push_back(absvalue & 0xff);
            absvalue >>= 8;
        }
        
        // If the most significant byte is >= 0x80, add another byte
        if (result.back() & 0x80) {
            result.push_back(neg ? 0x80 : 0x00);
        } else if (neg) {
            result.back() |= 0x80;
        }
        
        return result;
    }
};

RxdVMAdapter::RxdVMAdapter(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    const CRxdTx& tx,
    unsigned int inputIndex,
    uint32_t flags,
    std::shared_ptr<RxdExecutionContext> context)
    : impl_(std::make_unique<Impl>(scriptSig, scriptPubKey, tx, inputIndex, flags, context))
{
}

RxdVMAdapter::~RxdVMAdapter() = default;

bool RxdVMAdapter::Step() {
    return impl_->Step();
}

bool RxdVMAdapter::Run() {
    while (Step()) {}
    return impl_->currentState.success;
}

bool RxdVMAdapter::Rewind() {
    return impl_->Rewind();
}

void RxdVMAdapter::Reset() {
    impl_->Reset();
}

const VMState& RxdVMAdapter::GetState() const {
    return impl_->currentState;
}

bool RxdVMAdapter::IsDone() const {
    return impl_->currentState.done;
}

bool RxdVMAdapter::IsAtStart() const {
    return impl_->history.empty();
}

ScriptError RxdVMAdapter::GetError() const {
    return impl_->currentState.error;
}

std::string RxdVMAdapter::GetErrorString() const {
    return ScriptErrorString(impl_->currentState.error);
}

size_t RxdVMAdapter::GetHistoryDepth() const {
    return impl_->history.size();
}

void RxdVMAdapter::SetOpcodeCallback(OpcodeCallback callback) {
    impl_->opcodeCallback = std::move(callback);
}

void RxdVMAdapter::LoadArtifact(const RxdArtifact& artifact) {
    impl_->artifact = artifact;
}

std::optional<SourceMapEntry> RxdVMAdapter::GetCurrentSourceLocation() const {
    if (impl_->artifact.name.empty()) return std::nullopt;
    return impl_->artifact.GetSourceLocation(impl_->currentState.pc);
}

const CRxdTx& RxdVMAdapter::GetTransaction() const {
    return *impl_->tx;
}

unsigned int RxdVMAdapter::GetInputIndex() const {
    return impl_->inputIndex;
}

std::shared_ptr<RxdExecutionContext> RxdVMAdapter::GetContext() const {
    return impl_->context;
}

// High-level evaluation functions

bool EvalRxdScript(
    StackT& stack,
    const CRxdScript& script,
    uint32_t flags,
    std::shared_ptr<RxdExecutionContext> context,
    ScriptError* error)
{
    CRxdScript empty;
    CRxdTx dummyTx;
    RxdVMAdapter vm(empty, script, dummyTx, 0, flags, context);
    
    // Copy initial stack
    auto& state = const_cast<VMState&>(vm.GetState());
    state.stack = stack;
    
    bool result = vm.Run();
    stack = vm.GetState().stack;
    
    if (error) *error = vm.GetError();
    return result;
}

bool VerifyRxdScript(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    uint32_t flags,
    const CRxdTx& tx,
    unsigned int inputIndex,
    std::shared_ptr<RxdExecutionContext> context,
    ScriptError* error)
{
    RxdVMAdapter vm(scriptSig, scriptPubKey, tx, inputIndex, flags, context);
    bool result = vm.Run();
    if (error) *error = vm.GetError();
    return result;
}

} // namespace rxd
