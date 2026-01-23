// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_VM_ADAPTER_H
#define RXD_VM_ADAPTER_H

#include "rxd_script.h"
#include "rxd_params.h"
#include <functional>
#include <memory>
#include <optional>
#include <map>
#include <set>

namespace rxd {

// Forward declarations
class CRxdTx;
class RxdExecutionContext;

/**
 * Script error codes
 * Reference: Radiant-Core src/script/script_error.h
 */
enum class ScriptError {
    OK = 0,
    UNKNOWN,
    EVAL_FALSE,
    OP_RETURN,
    
    // Limits
    SCRIPT_SIZE,
    PUSH_SIZE,
    OP_COUNT,
    STACK_SIZE,
    SIG_COUNT,
    PUBKEY_COUNT,
    
    // Verify operations
    VERIFY,
    EQUALVERIFY,
    CHECKMULTISIGVERIFY,
    CHECKSIGVERIFY,
    NUMEQUALVERIFY,
    
    // Logic/format/canonical errors
    BAD_OPCODE,
    DISABLED_OPCODE,
    INVALID_STACK_OPERATION,
    INVALID_ALTSTACK_OPERATION,
    UNBALANCED_CONDITIONAL,
    
    // Signature errors
    SIG_HASHTYPE,
    SIG_DER,
    MINIMALDATA,
    SIG_PUSHONLY,
    SIG_HIGH_S,
    SIG_NULLDUMMY,
    PUBKEYTYPE,
    CLEANSTACK,
    MINIMALIF,
    SIG_NULLFAIL,
    
    // Locktime
    NEGATIVE_LOCKTIME,
    UNSATISFIED_LOCKTIME,
    
    // BIP62
    SIG_BADLENGTH,
    
    // Radiant-specific
    INVALID_REFERENCE,
    REFERENCE_NOT_FOUND,
    SINGLETON_MISMATCH,
    INVALID_STATE_SEPARATOR,
    INTROSPECTION_CONTEXT_UNAVAILABLE,
    
    ERROR_COUNT
};

/**
 * Get human-readable error string
 */
const char* ScriptErrorString(ScriptError error);

/**
 * VM state snapshot for debugging
 */
struct VMState {
    StackT stack;
    StackT altstack;
    CRxdScript script;
    size_t pc;                  // Program counter (byte offset)
    size_t opIndex;             // Opcode index (for display)
    size_t opCount;             // Number of opcodes executed
    bool done;
    bool success;
    ScriptError error;
    std::vector<bool> vfExec;   // Execution condition stack
    
    // Reference tracking state (for display)
    std::set<std::vector<uint8_t>> pushRefs;
    std::set<std::vector<uint8_t>> requireRefs;
    std::set<std::vector<uint8_t>> singletonRefs;
};

/**
 * Callback type for per-opcode notification
 */
using OpcodeCallback = std::function<void(
    opcodetype op,
    const valtype* pushData,    // nullptr if not a push
    const VMState& stateBefore,
    const VMState& stateAfter
)>;

/**
 * Source map entry for RadiantScript debugging
 */
struct SourceMapEntry {
    std::string file;
    uint32_t line;
    uint32_t column;
    std::string functionName;
};

/**
 * RadiantScript artifact for source-level debugging
 */
struct RxdArtifact {
    std::string name;
    std::string source;
    CRxdScript bytecode;
    std::map<size_t, SourceMapEntry> sourceMap;  // pc -> source location
    
    bool HasSourceMap() const { return !sourceMap.empty(); }
    std::optional<SourceMapEntry> GetSourceLocation(size_t pc) const;
};

/**
 * RxdVMAdapter - Main VM adapter for debugging
 * 
 * Provides step-by-step script execution with full state inspection.
 * Links to Radiant-Core for consensus-accurate execution.
 */
class RxdVMAdapter {
public:
    /**
     * Construct a VM adapter for script verification
     * 
     * @param scriptSig    Unlocking script (input)
     * @param scriptPubKey Locking script (output being spent)
     * @param tx           Transaction being validated
     * @param inputIndex   Index of input being validated
     * @param flags        Script verification flags
     * @param context      Execution context (for introspection)
     */
    RxdVMAdapter(
        const CRxdScript& scriptSig,
        const CRxdScript& scriptPubKey,
        const CRxdTx& tx,
        unsigned int inputIndex,
        uint32_t flags,
        std::shared_ptr<RxdExecutionContext> context
    );
    
    ~RxdVMAdapter();
    
    // Execution control
    
    /**
     * Execute one opcode and advance
     * @return true if execution can continue, false if done or error
     */
    bool Step();
    
    /**
     * Execute until completion
     * @return true if script succeeded, false otherwise
     */
    bool Run();
    
    /**
     * Rewind one step (if history available)
     * @return true if rewound, false if at start
     */
    bool Rewind();
    
    /**
     * Reset to initial state
     */
    void Reset();
    
    // State inspection
    
    /**
     * Get current VM state
     */
    const VMState& GetState() const;
    
    /**
     * Check if execution is complete
     */
    bool IsDone() const;
    
    /**
     * Check if at start (no history)
     */
    bool IsAtStart() const;
    
    /**
     * Get current error (if any)
     */
    ScriptError GetError() const;
    
    /**
     * Get error string
     */
    std::string GetErrorString() const;
    
    /**
     * Get execution history depth
     */
    size_t GetHistoryDepth() const;
    
    // Callbacks
    
    /**
     * Set callback for each opcode execution
     */
    void SetOpcodeCallback(OpcodeCallback callback);
    
    // Source-level debugging
    
    /**
     * Load RadiantScript artifact for source mapping
     */
    void LoadArtifact(const RxdArtifact& artifact);
    
    /**
     * Get current source location (if artifact loaded)
     */
    std::optional<SourceMapEntry> GetCurrentSourceLocation() const;
    
    // Context inspection
    
    /**
     * Get transaction being validated
     */
    const CRxdTx& GetTransaction() const;
    
    /**
     * Get input index being validated
     */
    unsigned int GetInputIndex() const;
    
    /**
     * Get execution context
     */
    std::shared_ptr<RxdExecutionContext> GetContext() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * High-level script evaluation (non-stepping)
 */
bool EvalRxdScript(
    StackT& stack,
    const CRxdScript& script,
    uint32_t flags,
    std::shared_ptr<RxdExecutionContext> context,
    ScriptError* error = nullptr
);

/**
 * High-level script verification (non-stepping)
 */
bool VerifyRxdScript(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    uint32_t flags,
    const CRxdTx& tx,
    unsigned int inputIndex,
    std::shared_ptr<RxdExecutionContext> context,
    ScriptError* error = nullptr
);

} // namespace rxd

#endif // RXD_VM_ADAPTER_H
