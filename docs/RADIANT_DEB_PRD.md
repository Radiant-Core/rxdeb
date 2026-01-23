# rxdeb: Product Requirements Document

**Version:** 1.1  
**Date:** January 23, 2026  
**Status:** Approved  

---

## Architecture Decisions (Finalized)

| Decision | Choice |
|----------|--------|
| **Radiant-Core Integration** | Option A: Git submodule (consensus accuracy) |
| **Repository** | New `rxdeb` repo (fresh fork from btcdeb) |
| **CLI Name** | `rxdeb` |
| **Network Priority** | Mainnet first, all networks supported |
| **Electrum Integration** | Required for v1.0 |
| **Source Debugging** | RadiantScript `.rad` source-level stepping |
| **Shared Constants** | `@radiantblockchain/constants` package |
| **radiantjs Strategy** | Full TypeScript rewrite |  

---

## Executive Summary

**rxdeb** is a fork of btcdeb (Bitcoin Script Debugger) that integrates Radiant Blockchain's extended Script engine while preserving btcdeb's excellent CLI front-end and REPL debugging experience. This tool will enable developers to step through Radiant scripts, inspect stack states, and debug smart contracts with the same intuitive workflow btcdeb provides for Bitcoin.

---

## 1. Project Overview

### 1.1 Background

**btcdeb** is the de-facto Bitcoin script debugger, offering:
- CLI-based script execution and debugging
- Step-by-step opcode execution with stack inspection
- Transaction context simulation

**Note:** rxdeb does NOT support SegWit, Taproot, or Tapscript. Radiant uses a non-SegWit transaction format with BIP143-style sighash and mandatory SIGHASH_FORKID.

**Radiant Blockchain** extends Bitcoin's scripting capabilities significantly:
- **50+ new opcodes** including native introspection, reference-based tokens, and state management
- **64-bit integer arithmetic** (vs Bitcoin's 32-bit limitation)
- **32MB script/stack limits** (vs Bitcoin's 520-byte element size)
- **UTXO references** for on-chain token protocols
- **State separators** for contract state/code distinction

### 1.2 Goals

1. **Preserve btcdeb UX**: Maintain the CLI/REPL interface developers love
2. **Full Radiant Script Support**: All Radiant opcodes with correct semantics
3. **Consensus Accuracy**: Execute scripts identically to Radiant-Core
4. **Developer Experience**: Rich debugging for Radiant smart contracts
5. **Test Infrastructure**: Comprehensive test vectors for Radiant-specific features

### 1.3 Non-Goals

- GUI interface (future consideration)
- Full node functionality
- Wallet features

---

## 2. Technical Analysis

### 2.1 Key Differences: Bitcoin vs Radiant Script

| Feature | Bitcoin (btcdeb) | Radiant |
|---------|------------------|---------|
| Max script element size | 520 bytes | 32,000,000 bytes |
| Max script size | 10,000 bytes | 32,000,000 bytes |
| Max stack size | 1,000 elements | 32,000,000 elements |
| Max ops per script | 201 | 32,000,000 |
| Integer size | 32-bit (4 bytes) | 64-bit (8 bytes) |
| SigHash | SIGHASH_FORKID optional | SIGHASH_FORKID required |
| SegWit/Taproot | Yes | No |
| Native Introspection | No | Yes (OP_INPUTINDEX, OP_TXVERSION, etc.) |
| Reference Opcodes | No | Yes (OP_PUSHINPUTREF, etc.) |
| State Separator | No | Yes (OP_STATESEPARATOR) |

### 2.2 Radiant-Specific Opcodes (Not in Bitcoin)

#### Native Introspection (0xC0-0xCD)
```
OP_INPUTINDEX (0xC0)      - Push current input index
OP_ACTIVEBYTECODE (0xC1)  - Push executing script bytecode
OP_TXVERSION (0xC2)       - Push transaction version
OP_TXINPUTCOUNT (0xC3)    - Push number of inputs
OP_TXOUTPUTCOUNT (0xC4)   - Push number of outputs
OP_TXLOCKTIME (0xC5)      - Push transaction locktime
OP_UTXOVALUE (0xC6)       - Push UTXO value at index
OP_UTXOBYTECODE (0xC7)    - Push UTXO scriptPubKey at index
OP_OUTPOINTTXHASH (0xC8)  - Push outpoint txid at index
OP_OUTPOINTINDEX (0xC9)   - Push outpoint vout at index
OP_INPUTBYTECODE (0xCA)   - Push input scriptSig at index
OP_INPUTSEQUENCENUMBER (0xCB) - Push input sequence at index
OP_OUTPUTVALUE (0xCC)     - Push output value at index
OP_OUTPUTBYTECODE (0xCD)  - Push output scriptPubKey at index
```

#### SHA512/256 Functions (0xCE-0xCF)
```
OP_SHA512_256 (0xCE)      - SHA512/256 hash
OP_HASH512_256 (0xCF)     - Double SHA512/256 hash
```

#### Reference Opcodes (0xD0-0xEC)
```
OP_PUSHINPUTREF (0xD0)           - Push and track reference
OP_REQUIREINPUTREF (0xD1)        - Require reference exists
OP_DISALLOWPUSHINPUTREF (0xD2)   - Disallow reference propagation
OP_DISALLOWPUSHINPUTREFSIBLING (0xD3) - Disallow sibling refs
OP_REFHASHDATASUMMARY_UTXO (0xD4)
OP_REFHASHVALUESUM_UTXOS (0xD5)
OP_REFHASHDATASUMMARY_OUTPUT (0xD6)
OP_REFHASHVALUESUM_OUTPUTS (0xD7)
OP_PUSHINPUTREFSINGLETON (0xD8)
OP_REFTYPE_UTXO (0xD9)
OP_REFTYPE_OUTPUT (0xDA)
OP_REFVALUESUM_UTXOS (0xDB)
OP_REFVALUESUM_OUTPUTS (0xDC)
OP_REFOUTPUTCOUNT_UTXOS (0xDD)
OP_REFOUTPUTCOUNT_OUTPUTS (0xDE)
OP_REFOUTPUTCOUNTZEROVALUED_UTXOS (0xDF)
OP_REFOUTPUTCOUNTZEROVALUED_OUTPUTS (0xE0)
OP_REFDATASUMMARY_UTXO (0xE1)
OP_REFDATASUMMARY_OUTPUT (0xE2)
OP_CODESCRIPTHASHVALUESUM_UTXOS (0xE3)
OP_CODESCRIPTHASHVALUESUM_OUTPUTS (0xE4)
OP_CODESCRIPTHASHOUTPUTCOUNT_UTXOS (0xE5)
OP_CODESCRIPTHASHOUTPUTCOUNT_OUTPUTS (0xE6)
OP_CODESCRIPTHASHZEROVALUEDOUTPUTCOUNT_UTXOS (0xE7)
OP_CODESCRIPTHASHZEROVALUEDOUTPUTCOUNT_OUTPUTS (0xE8)
OP_CODESCRIPTBYTECODE_UTXO (0xE9)
OP_CODESCRIPTBYTECODE_OUTPUT (0xEA)
OP_STATESCRIPTBYTECODE_UTXO (0xEB)
OP_STATESCRIPTBYTECODE_OUTPUT (0xEC)
OP_PUSH_TX_STATE (0xED)
```

#### State Management (0xBD-0xBF)
```
OP_STATESEPARATOR (0xBD)         - Separates state from code
OP_STATESEPARATORINDEX_UTXO (0xBE)
OP_STATESEPARATORINDEX_OUTPUT (0xBF)
```

#### Re-enabled Opcodes (disabled in Bitcoin)
```
OP_CAT (0x7E)    - Concatenate strings
OP_SPLIT (0x7F)  - Split string
OP_AND (0x84)    - Bitwise AND
OP_OR (0x85)     - Bitwise OR
OP_XOR (0x86)    - Bitwise XOR
OP_DIV (0x96)    - Integer division
OP_MOD (0x97)    - Modulo
OP_MUL (0x95)    - Multiplication
OP_NUM2BIN (0x80)
OP_BIN2NUM (0x81)
OP_CHECKDATASIG (0xBA)
OP_CHECKDATASIGVERIFY (0xBB)
OP_REVERSEBYTES (0xBC)
```

### 2.3 Script Execution Context

Radiant requires a rich execution context for introspection opcodes. The `ScriptExecutionContext` from Radiant-Core includes:

- Transaction being validated
- All input coins (UTXOs being spent)
- Reference tracking maps (refAssetId → Amount, refHash → Amount)
- Code script hash tracking
- State separator indices

---

## 3. Architecture

### 3.1 High-Level Design

```
┌─────────────────────────────────────────────────────────────┐
│                    radiant-deb CLI                          │
│  (btcdeb CLI preserved, extended with --rxd flags)          │
├─────────────────────────────────────────────────────────────┤
│                    Backend Router                           │
│  ┌─────────────────────┐    ┌─────────────────────┐        │
│  │   Bitcoin Backend   │    │   Radiant Backend   │        │
│  │   (existing btcdeb) │    │   (new rxd/ layer)  │        │
│  └─────────────────────┘    └─────────────────────┘        │
├─────────────────────────────────────────────────────────────┤
│                    Shared Components                        │
│  - Crypto (secp256k1, SHA256, RIPEMD160)                   │
│  - Serialization (streams, hex encoding)                    │
│  - REPL infrastructure (kerl)                               │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Directory Structure

```
rxdeb/
├── rxd/                          # NEW: Radiant adapter layer
│   ├── rxd_params.h/.cpp         # Network params, limits, constants
│   ├── rxd_script.h/.cpp         # Script wrapper, opcode enum
│   ├── rxd_tx.h/.cpp             # Transaction types
│   ├── rxd_vm_adapter.h/.cpp     # VM stepping interface
│   ├── rxd_context.h/.cpp        # ScriptExecutionContext adapter
│   ├── rxd_opcodes.h/.cpp        # Opcode implementations
│   └── rxd_sighash.h/.cpp        # Radiant sighash computation
├── script/                        # Modified: Backend routing
│   ├── interpreter.cpp            # Add Radiant dispatch
│   └── script.h                   # Extended opcode enum
├── debugger/
│   ├── interpreter.h/.cpp         # Add RxdInterpreterEnv
│   └── see.h/.cpp                 # Stack/environment display
├── instance.h/.cpp                # Add RxdInstance
├── btcdeb.cpp                     # Add --rxd CLI flags
├── tests/
│   └── rxd/                       # NEW: Radiant test vectors
│       ├── rxd_script_tests.cpp
│       ├── rxd_vm_tests.cpp
│       ├── rxd_introspection_tests.cpp
│       ├── rxd_reference_tests.cpp
│       └── data/
│           ├── valid_txs.json
│           ├── invalid_txs.json
│           └── scripts.json
└── examples/
    └── rxd/                       # NEW: Radiant examples
        ├── rxd_basic_p2pkh.txt
        ├── rxd_fungible_token.txt
        ├── rxd_introspection.txt
        └── README.md
```

---

## 4. Implementation Specifications

### 4.1 New Files: `rxd/` Directory

#### 4.1.1 `rxd/rxd_params.h`

```cpp
#ifndef RXD_PARAMS_H
#define RXD_PARAMS_H

#include <cstdint>
#include <string>

namespace rxd {

enum class Network {
    MAINNET,
    TESTNET,
    REGTEST
};

struct ChainParams {
    std::string name;
    uint8_t pubkeyPrefix;
    uint8_t scriptPrefix;
    uint32_t magicBytes;
    uint16_t defaultPort;
    
    // Script limits (Radiant-specific)
    static constexpr uint32_t MAX_SCRIPT_ELEMENT_SIZE = 32000000;
    static constexpr uint32_t MAX_SCRIPT_SIZE = 32000000;
    static constexpr uint32_t MAX_STACK_SIZE = 32000000;
    static constexpr uint32_t MAX_OPS_PER_SCRIPT = 32000000;
    static constexpr size_t MAX_SCRIPTNUM_SIZE = 8;  // 64-bit
};

const ChainParams& GetParams(Network network);

// Script verification flags
enum ScriptFlags : uint32_t {
    SCRIPT_VERIFY_NONE = 0,
    SCRIPT_VERIFY_P2SH = (1U << 0),
    SCRIPT_VERIFY_STRICTENC = (1U << 1),
    SCRIPT_VERIFY_DERSIG = (1U << 2),
    SCRIPT_VERIFY_LOW_S = (1U << 3),
    SCRIPT_VERIFY_SIGPUSHONLY = (1U << 5),
    SCRIPT_VERIFY_MINIMALDATA = (1U << 6),
    SCRIPT_VERIFY_CLEANSTACK = (1U << 8),
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9),
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10),
    SCRIPT_VERIFY_MINIMALIF = (1U << 13),
    SCRIPT_VERIFY_NULLFAIL = (1U << 14),
    SCRIPT_ENABLE_SIGHASH_FORKID = (1U << 16),
    SCRIPT_64_BIT_INTEGERS = (1U << 24),
    SCRIPT_NATIVE_INTROSPECTION = (1U << 25),
    SCRIPT_ENHANCED_REFERENCES = (1U << 26),
    SCRIPT_PUSH_TX_STATE = (1U << 27),
};

constexpr uint32_t STANDARD_SCRIPT_VERIFY_FLAGS = 
    SCRIPT_VERIFY_P2SH |
    SCRIPT_VERIFY_STRICTENC |
    SCRIPT_VERIFY_DERSIG |
    SCRIPT_VERIFY_LOW_S |
    SCRIPT_VERIFY_SIGPUSHONLY |
    SCRIPT_VERIFY_MINIMALDATA |
    SCRIPT_VERIFY_CLEANSTACK |
    SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
    SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
    SCRIPT_VERIFY_MINIMALIF |
    SCRIPT_VERIFY_NULLFAIL |
    SCRIPT_ENABLE_SIGHASH_FORKID |
    SCRIPT_64_BIT_INTEGERS |
    SCRIPT_NATIVE_INTROSPECTION |
    SCRIPT_ENHANCED_REFERENCES;

} // namespace rxd

#endif // RXD_PARAMS_H
```

#### 4.1.2 `rxd/rxd_script.h`

```cpp
#ifndef RXD_SCRIPT_H
#define RXD_SCRIPT_H

#include <script/script.h>
#include <vector>
#include <string>

namespace rxd {

// Extended opcode enumeration for Radiant
enum opcodetype : uint8_t {
    // ... (include all Bitcoin opcodes)
    
    // Radiant-specific opcodes
    OP_STATESEPARATOR = 0xbd,
    OP_STATESEPARATORINDEX_UTXO = 0xbe,
    OP_STATESEPARATORINDEX_OUTPUT = 0xbf,
    
    // Native Introspection
    OP_INPUTINDEX = 0xc0,
    OP_ACTIVEBYTECODE = 0xc1,
    OP_TXVERSION = 0xc2,
    OP_TXINPUTCOUNT = 0xc3,
    OP_TXOUTPUTCOUNT = 0xc4,
    OP_TXLOCKTIME = 0xc5,
    OP_UTXOVALUE = 0xc6,
    OP_UTXOBYTECODE = 0xc7,
    OP_OUTPOINTTXHASH = 0xc8,
    OP_OUTPOINTINDEX = 0xc9,
    OP_INPUTBYTECODE = 0xca,
    OP_INPUTSEQUENCENUMBER = 0xcb,
    OP_OUTPUTVALUE = 0xcc,
    OP_OUTPUTBYTECODE = 0xcd,
    
    // SHA512/256
    OP_SHA512_256 = 0xce,
    OP_HASH512_256 = 0xcf,
    
    // Reference opcodes
    OP_PUSHINPUTREF = 0xd0,
    OP_REQUIREINPUTREF = 0xd1,
    OP_DISALLOWPUSHINPUTREF = 0xd2,
    OP_DISALLOWPUSHINPUTREFSIBLING = 0xd3,
    // ... (all reference opcodes through 0xed)
    OP_PUSH_TX_STATE = 0xed,
};

const char* GetRxdOpName(opcodetype opcode);
bool IsRxdOpcode(opcodetype opcode);
bool IsIntrospectionOpcode(opcodetype opcode);
bool IsReferenceOpcode(opcodetype opcode);

// CScript wrapper with Radiant extensions
class CRxdScript : public CScript {
public:
    using CScript::CScript;
    
    bool HasStateSeparator() const;
    uint32_t GetStateSeparatorIndex() const;
    CRxdScript GetStateScript() const;
    CRxdScript GetCodeScript() const;
};

} // namespace rxd

#endif // RXD_SCRIPT_H
```

#### 4.1.3 `rxd/rxd_vm_adapter.h`

```cpp
#ifndef RXD_VM_ADAPTER_H
#define RXD_VM_ADAPTER_H

#include <rxd/rxd_script.h>
#include <rxd/rxd_tx.h>
#include <rxd/rxd_context.h>
#include <vector>
#include <functional>

namespace rxd {

using valtype = std::vector<uint8_t>;
using StackT = std::vector<valtype>;

// Callback for per-opcode stepping
using OpcodeCallback = std::function<void(
    opcodetype op,
    const StackT& stack,
    const StackT& altstack,
    size_t pc
)>;

struct VMState {
    StackT stack;
    StackT altstack;
    CRxdScript script;
    size_t pc;
    bool done;
    bool success;
    ScriptError error;
    std::vector<bool> vfExec;
};

class RxdVMAdapter {
public:
    RxdVMAdapter(
        const CRxdScript& scriptSig,
        const CRxdScript& scriptPubKey,
        const CRxdTx& tx,
        unsigned int inputIndex,
        uint32_t flags,
        const RxdExecutionContext& context
    );
    
    // Single step execution
    bool Step();
    
    // Run to completion
    bool Run();
    
    // State inspection
    const VMState& GetState() const;
    
    // Register callback for each opcode
    void SetOpcodeCallback(OpcodeCallback cb);
    
    // Rewind support
    bool CanRewind() const;
    bool Rewind();
    
private:
    VMState state_;
    std::vector<VMState> history_;
    OpcodeCallback callback_;
    RxdExecutionContext context_;
    uint32_t flags_;
};

// High-level evaluation function
bool EvalRxdScript(
    StackT& stack,
    const CRxdScript& script,
    uint32_t flags,
    const BaseSignatureChecker& checker,
    const RxdExecutionContext& context,
    ScriptError* error = nullptr
);

bool VerifyRxdScript(
    const CRxdScript& scriptSig,
    const CRxdScript& scriptPubKey,
    uint32_t flags,
    const BaseSignatureChecker& checker,
    const RxdExecutionContext& context,
    ScriptError* error = nullptr
);

} // namespace rxd

#endif // RXD_VM_ADAPTER_H
```

### 4.2 Modified Files

#### 4.2.1 `instance.h` Modifications

```cpp
// Add to existing Instance class or create RxdInstance

enum class Backend {
    BITCOIN,
    RADIANT
};

class RxdInstance {
public:
    rxd::RxdVMAdapter* vm;
    rxd::RxdExecutionContext context;
    rxd::Network network;
    rxd::CRxdTx tx;
    rxd::CRxdTx txin;
    int64_t txin_index;
    int64_t txin_vout_index;
    std::vector<Amount> amounts;
    rxd::CRxdScript script;
    std::vector<valtype> stack;
    ScriptError error;
    
    RxdInstance();
    ~RxdInstance();
    
    bool parse_transaction(const char* txdata);
    bool parse_input_transaction(const char* txdata, int select_index = -1);
    bool parse_script(const char* script_str);
    bool setup_environment(uint32_t flags = rxd::STANDARD_SCRIPT_VERIFY_FLAGS);
    
    bool step(size_t steps = 1);
    bool rewind();
    bool at_end();
    bool at_start();
    std::string error_string();
};
```

#### 4.2.2 `btcdeb.cpp` CLI Modifications

```cpp
// Add new CLI options
ca.add_option("rxd", 'R', no_arg);
ca.add_option("network", 'N', req_arg);  // rxd-main, rxd-test, rxd-regtest
ca.add_option("context", 'C', req_arg);  // JSON file with full tx context

// In main():
bool use_radiant = ca.m.count('R');
rxd::Network rxd_network = rxd::Network::MAINNET;

if (ca.m.count('N')) {
    std::string net = ca.m['N'];
    if (net == "rxd-main") rxd_network = rxd::Network::MAINNET;
    else if (net == "rxd-test") rxd_network = rxd::Network::TESTNET;
    else if (net == "rxd-regtest") rxd_network = rxd::Network::REGTEST;
}

if (use_radiant) {
    // Use RxdInstance instead of Instance
    RxdInstance rxd_instance;
    rxd_instance.network = rxd_network;
    // ... setup and run
}
```

### 4.3 Test Fixtures

#### 4.3.1 `tests/rxd/data/scripts.json`

```json
[
  {
    "description": "Basic P2PKH",
    "script_asm": "OP_DUP OP_HASH160 <pubkeyhash> OP_EQUALVERIFY OP_CHECKSIG",
    "initial_stack": ["<sig>", "<pubkey>"],
    "expected_result": true,
    "expected_final_stack": ["01"]
  },
  {
    "description": "OP_INPUTINDEX introspection",
    "script_asm": "OP_INPUTINDEX OP_0 OP_EQUAL",
    "context": {"input_index": 0},
    "expected_result": true
  },
  {
    "description": "OP_TXINPUTCOUNT introspection",
    "script_asm": "OP_TXINPUTCOUNT OP_2 OP_EQUAL",
    "context": {"tx_input_count": 2},
    "expected_result": true
  },
  {
    "description": "64-bit arithmetic",
    "script_asm": "OP_1 21000000000000 OP_ADD 21000000000001 OP_EQUAL",
    "expected_result": true
  },
  {
    "description": "OP_CAT string concatenation",
    "script_asm": "0x0102 0x0304 OP_CAT 0x01020304 OP_EQUAL",
    "expected_result": true
  },
  {
    "description": "State separator",
    "script_asm": "OP_1 OP_STATESEPARATOR OP_1 OP_EQUAL",
    "expected_result": true
  }
]
```

---

## 5. Integration Strategy

### 5.1 Radiant-Core Integration Options

**Selected: Git Submodule Approach**
- Add Radiant-Core as a git submodule under `deps/radiant-core`
- Link against its interpreter library for 100% consensus compatibility
- Always in sync with consensus rules
- Build system will compile required Radiant-Core components

### 5.2 Build System

```makefile
# Makefile.am additions
if ENABLE_RXD
RXD_SOURCES = \
    rxd/rxd_params.cpp \
    rxd/rxd_script.cpp \
    rxd/rxd_tx.cpp \
    rxd/rxd_vm_adapter.cpp \
    rxd/rxd_context.cpp \
    rxd/rxd_opcodes.cpp \
    rxd/rxd_sighash.cpp

btcdeb_SOURCES += $(RXD_SOURCES)
btcdeb_CPPFLAGS += -DRXD_SUPPORT
endif
```

---

## 6. Development Roadmap

### Phase 1: Foundation (Weeks 1-3)
- [ ] Fork btcdeb repository
- [ ] Create `rxd/` directory structure
- [ ] Implement `rxd_params.h/.cpp` with constants
- [ ] Implement `rxd_script.h/.cpp` with opcode enum
- [ ] Add `--rxd` CLI flag (no-op initially)
- [ ] Set up test infrastructure

### Phase 2: Core Script Engine (Weeks 4-6)
- [ ] Implement basic opcode execution (push, stack ops, arithmetic)
- [ ] Port 64-bit CScriptNum from Radiant-Core
- [ ] Implement re-enabled opcodes (OP_CAT, OP_MUL, etc.)
- [ ] Create basic script tests
- [ ] Implement VM stepping interface

### Phase 3: Transaction Support (Weeks 7-8)
- [ ] Implement `CRxdTx` transaction parsing
- [ ] Implement Radiant sighash computation
- [ ] Implement signature verification
- [ ] Add transaction-based tests

### Phase 4: Introspection Opcodes (Weeks 9-11)
- [ ] Implement `RxdExecutionContext`
- [ ] Implement all native introspection opcodes
- [ ] Implement state separator support
- [ ] Add introspection tests

### Phase 5: Reference Opcodes (Weeks 12-14)
- [ ] Implement reference tracking structures
- [ ] Implement all OP_PUSHINPUTREF variants
- [ ] Implement reference query opcodes
- [ ] Add reference tests

### Phase 6: Integration & Polish (Weeks 15-16)
- [ ] Full REPL integration
- [ ] Example scripts and documentation
- [ ] Performance optimization
- [ ] Cross-reference with Radiant-Core test vectors
- [ ] Release candidate

---

## 7. Ecosystem Integration

### 7.1 RadiantScript Compiler Integration

RadiantScript (fork of CashScript) compiles high-level contracts to Radiant bytecode. Integration points:

- **radiant-deb** can debug compiled RadiantScript artifacts
- Artifact JSON includes source maps for debugging
- CLI can accept `.json` artifact files directly

```bash
# Compile contract with source maps
npx radc FungibleToken.rad -o FungibleToken.json --debug

# Debug compiled contract with source-level stepping
rxdeb --artifact=FungibleToken.json --tx=<hex>

# Step through .rad source code (not just bytecode)
rxdeb> step    # Shows corresponding .rad source line
rxdeb> source  # Display current position in .rad file
```

### 7.2 radiantjs Integration

radiantjs is the JavaScript library for Radiant transactions. Integration points:

- Generate test transactions for radiant-deb
- Export transaction hex for debugging
- Shared opcode constants

### 7.3 ElectrumX-Core Integration

For debugging scripts against live UTXOs:

```bash
# Fetch UTXO context from Electrum and debug (Required for v1.0)
rxdeb --electrum=localhost:50001 --txid=<txid> --vin=0

# Or use mainnet public Electrum
rxdeb --electrum=electrum.radiant.ovh:50002 --txid=<txid> --vin=0

# Debug with full transaction context from network
rxdeb --electrum=<host:port> --address=<contract_address> --latest
```

---

## 8. Quality Assurance

### 8.1 Test Categories

1. **Unit Tests**: Individual opcode behavior
2. **Integration Tests**: Full script execution
3. **Consensus Tests**: Compare with Radiant-Core output
4. **Regression Tests**: Known edge cases
5. **Fuzz Tests**: Random script generation

### 8.2 Golden Test Generation

```bash
# Generate golden tests from Radiant-Core
./generate-golden-tests.py \
    --node=localhost:7332 \
    --output=tests/rxd/data/golden_tests.json
```

### 8.3 CI/CD Pipeline

```yaml
# .github/workflows/test.yml
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build
        run: |
          ./autogen.sh
          ./configure --enable-rxd
          make
      - name: Run Bitcoin Tests
        run: make check
      - name: Run Radiant Tests
        run: make check-rxd
```

---

## 9. Clarifying Questions

Before proceeding with implementation, please clarify:

1. **Radiant-Core Linking**: Should we link against Radiant-Core as a library (ensures consensus) or maintain a standalone implementation (lighter weight)?

2. **Network Support Priority**: Which networks are priority? (mainnet, testnet, regtest)

3. **Electrum Integration**: Is live UTXO fetching via Electrum a requirement for v1.0?

4. **RadiantScript Source Maps**: Should we support stepping through RadiantScript source code (not just bytecode)?

5. **Reference Visualization**: For reference opcodes, what visualization would be most helpful? (reference graph, token tracking, etc.)

6. **Performance Requirements**: Any specific performance targets for large scripts?

---

## 10. Suggestions

### 10.1 Quick Wins

1. **Start with introspection opcodes** - They're relatively simple and immediately useful
2. **Use Radiant-Core's test vectors** - Leverage existing `script_tests.cpp` and `native_introspection_tests.cpp`
3. **Build example library early** - Helps validate the tool with real use cases

### 10.2 Future Enhancements

1. **Web UI**: Browser-based debugger using WASM
2. **VSCode Extension**: Integrated debugging in IDE
3. **Script Profiler**: Gas/op counting for optimization
4. **Contract Templates**: Pre-built templates for common patterns

### 10.3 Ecosystem Synergies

1. **RadiantScript**: Add `--debug` flag that outputs radiant-deb compatible format
2. **radiantjs**: Add `Transaction.toDebugHex()` method
3. **Photonic Wallet**: Integration for debugging wallet transactions

---

## 11. Appendix

### A. Reference Implementation Files (Radiant-Core)

Key files to reference during implementation:

| File | Purpose |
|------|---------|
| `src/script/script.h` | Opcode definitions, CScript class |
| `src/script/interpreter.cpp` | Script execution (143KB) |
| `src/script/script_execution_context.h` | Introspection context |
| `src/script/script_flags.h` | Verification flags |
| `src/script/sighashtype.h` | Sighash types |
| `src/primitives/transaction.h` | Transaction structures |
| `src/test/native_introspection_tests.cpp` | Test vectors |
| `src/test/script_tests.cpp` | Script test vectors |

### B. Opcode Byte Map

```
0x00-0x4B: Push operations
0x4C-0x4E: PUSHDATA variants
0x4F-0x60: Push number (-1 through 16)
0x61-0x6A: Flow control
0x6B-0x7D: Stack operations
0x7E-0x82: Splice operations (re-enabled in Radiant)
0x83-0x8A: Bitwise operations
0x8B-0xA5: Arithmetic operations
0xA6-0xAF: Crypto operations
0xB0-0xB9: Locktime operations
0xBA-0xBC: CHECKDATASIG, REVERSEBYTES
0xBD-0xBF: State separator operations
0xC0-0xCD: Native introspection
0xCE-0xCF: SHA512/256
0xD0-0xED: Reference operations
```

---

**Document Revision History**

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-23 | Cascade | Initial PRD |
