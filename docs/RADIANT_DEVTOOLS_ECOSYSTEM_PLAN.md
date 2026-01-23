# Radiant Developer Tools Ecosystem Upgrade Plan

**Version:** 1.1  
**Date:** January 23, 2026  
**Status:** Approved  

---

## Finalized Decisions

| Decision | Choice |
|----------|--------|
| **rxdeb Integration** | Git submodule linking to Radiant-Core |
| **Shared Constants** | Create `@radiantblockchain/constants` package |
| **radiantjs** | Full TypeScript rewrite |
| **Source Debugging** | RadiantScript `.rad` source-level stepping required |
| **Electrum** | Required for rxdeb v1.0 |  

---

## Executive Summary

This document outlines a comprehensive plan to upgrade the Radiant blockchain developer tools ecosystem. The ecosystem consists of interconnected projects that need coordinated improvements to provide a world-class developer experience.

---

## 1. Ecosystem Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        RADIANT DEVELOPER ECOSYSTEM                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │ RadiantScript│───▶│  radiant-deb │◀───│   radiantjs  │              │
│  │  (Compiler)  │    │  (Debugger)  │    │   (JS SDK)   │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│         │                   │                   │                       │
│         ▼                   ▼                   ▼                       │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │   Artifact   │    │   Test Tx    │    │ Transaction  │              │
│  │    .json     │    │   Vectors    │    │   Builder    │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│         │                   │                   │                       │
│         └───────────────────┼───────────────────┘                       │
│                             ▼                                           │
│                    ┌──────────────┐                                     │
│                    │ Radiant-Core │                                     │
│                    │    (Node)    │                                     │
│                    └──────────────┘                                     │
│                             │                                           │
│                             ▼                                           │
│                    ┌──────────────┐                                     │
│                    │ ElectrumX-   │                                     │
│                    │    Core      │                                     │
│                    └──────────────┘                                     │
│                             │                                           │
│         ┌───────────────────┼───────────────────┐                       │
│         ▼                   ▼                   ▼                       │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │   Electron   │    │   Photonic   │    │    Glyph     │              │
│  │    Wallet    │    │    Wallet    │    │    Miner     │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.1 Project Inventory

| Project | Location | Purpose | Current State |
|---------|----------|---------|---------------|
| **Radiant-Core** | `/Downloads/Radiant-Core-main` | Full node, consensus | Stable |
| **radiant-deb** | `/Downloads/btcdeb-master` (fork source) | Script debugger | New project |
| **RadiantScript** | `/Downloads/RadiantScript-radiantscript` | High-level compiler | Alpha |
| **radiantjs** | `/Downloads/radiantjs-master` | JavaScript SDK | Stable, needs updates |
| **ElectrumX-Core** | `/Downloads/ElectrumX-Core` | Indexer server | Stable |
| **Electron-Wallet** | `/Downloads/Electron-Wallet-master` | Desktop wallet | Stable |
| **Photonic-Wallet** | `/Downloads/Photonic-Wallet-master` | Web wallet | Active |
| **Glyph-miner** | `/Downloads/Glyph-miner-master-2` | Token miner | Active |

---

## 2. Priority Matrix

### 2.1 Project Priority Ranking

| Priority | Project | Rationale |
|----------|---------|-----------|
| **P0** | rxdeb | Foundational debugger enables all other development |
| **P1** | RadiantScript | High-level language adoption driver |
| **P1** | radiantjs | Core SDK for all JS applications |
| **P2** | ElectrumX-Core | Infrastructure for all wallets/apps |
| **P3** | Wallets/Miners | End-user applications |

### 2.2 Dependency Graph

```
rxdeb (P0)
    └── Enables debugging for RadiantScript contracts
    └── Provides test vectors for radiantjs
    └── Electrum integration for live UTXO debugging
    
@radiantblockchain/constants (P0)
    └── Shared opcodes, limits, flags
    └── Consumed by: rxdeb, radiantjs, RadiantScript
    
RadiantScript (P1)
    └── Depends on: radiant-deb for debugging
    └── Outputs to: radiantjs for deployment
    
radiantjs (P1)
    └── Depends on: Radiant-Core opcodes
    └── Consumed by: Wallets, dApps, radiant-deb
    
ElectrumX-Core (P2)
    └── Depends on: Radiant-Core
    └── Consumed by: All wallets and applications
```

---

## 3. Project-Specific Upgrade Plans

---

### 3.1 radiant-deb (P0) - NEW PROJECT

**See:** `RADIANT_DEB_PRD.md` for full specification.

**Summary:**
- Fork btcdeb, add Radiant backend
- Support all 50+ Radiant opcodes
- Full introspection and reference debugging
- 16-week development timeline

**Key Deliverables:**
1. `rxd/` adapter layer
2. Radiant opcode support
3. Test vector suite
4. Example scripts

---

### 3.2 RadiantScript Upgrades (P1)

**Current State:** Alpha, fork of CashScript

**Location:** `/Downloads/RadiantScript-radiantscript`

#### 3.2.1 Identified Issues

1. **Incomplete Opcode Coverage**
   - Missing some newer Radiant opcodes
   - Reference opcodes need better syntax

2. **Limited Debugging Support**
   - No source maps in artifacts
   - No integration with debuggers

3. **Documentation Gaps**
   - Sparse examples
   - Missing API documentation

4. **Type System Limitations**
   - No custom types for refs
   - Limited struct support

#### 3.2.2 Proposed Upgrades

##### Phase 1: Opcode Completeness (2 weeks)
```
- [ ] Audit current opcode coverage vs Radiant-Core
- [ ] Add missing opcodes to grammar
- [ ] Update code generator for new opcodes
- [ ] Add opcode tests
```

##### Phase 2: Debugging Integration (3 weeks)
```
- [ ] Add source map generation to compiler
- [ ] Create --debug flag for verbose output
- [ ] Output radiant-deb compatible format
- [ ] Add breakpoint annotations
```

##### Phase 3: Developer Experience (2 weeks)
```
- [ ] Add ref type to type system
- [ ] Improve error messages with suggestions
- [ ] Add LSP (Language Server Protocol) support
- [ ] Create VSCode extension
```

##### Phase 4: Documentation (1 week)
```
- [ ] API documentation with TypeDoc
- [ ] Tutorial series (5+ tutorials)
- [ ] Example contract library (10+ examples)
- [ ] Video walkthroughs
```

#### 3.2.3 New Syntax Proposals

```solidity
// Current syntax for references
bytes36 ref = pushInputRef(REF);

// Proposed enhanced syntax
ref token = pushInputRef(REF);  // New 'ref' type
singleton nft = pushInputRefSingleton(REF);  // Singleton type

// Introspection improvements
require(tx.inputs.count >= 2);  // More readable
require(tx.outputs[0].value >= 1000);

// State management
state {
    bytes20 ownerPkh;
    int64 balance;
}

code {
    function transfer(sig s, pubkey pk, int64 amount) {
        require(hash160(pk) == state.ownerPkh);
        require(checkSig(s, pk));
        require(amount <= state.balance);
    }
}
```

---

### 3.3 radiantjs Upgrades (P1)

**Current State:** Stable but dated, forked from bsv.js

**Location:** `/Downloads/radiantjs-master`

#### 3.3.1 Identified Issues

1. **Incomplete Opcode Support**
   - Script class missing Radiant-specific opcodes
   - No helpers for reference operations

2. **Outdated Architecture**
   - CommonJS only, no ESM
   - No TypeScript definitions beyond basic `.d.ts`
   - Large bundle size (400KB+ minified)

3. **Missing Features**
   - No PSBT support
   - No contract interaction helpers
   - Limited transaction introspection

4. **Testing Gaps**
   - Some tests commented out
   - No integration tests with live network

#### 3.3.2 Proposed Upgrades

##### Phase 1: Opcode & Script Modernization (3 weeks)
```javascript
// Current: Limited opcode support
Script.Opcode.OP_CHECKSIG

// Proposed: Full Radiant opcode support
Script.Opcode.OP_PUSHINPUTREF        // 0xD0
Script.Opcode.OP_INPUTINDEX          // 0xC0
Script.Opcode.OP_TXVERSION           // 0xC2
Script.Opcode.OP_STATESEPARATOR      // 0xBD
// ... all 50+ Radiant opcodes

// New helper methods
Script.buildPushInputRef(refBytes)
Script.buildIntrospection(opcode, index)
Script.parseStateSeparator(script)
```

##### Phase 2: TypeScript Migration (4 weeks)
```
- [ ] Convert codebase to TypeScript
- [ ] Add strict type definitions
- [ ] Export ESM and CJS builds
- [ ] Tree-shakeable exports
- [ ] Reduce bundle size to <100KB
```

##### Phase 3: New Features (3 weeks)
```typescript
// Contract interaction helpers
class Contract {
  constructor(artifact: Artifact, utxos: UTXO[]);
  
  // Call contract function
  async call(functionName: string, args: any[]): Promise<Transaction>;
  
  // Build unlock script
  getUnlockScript(functionName: string, args: any[]): Script;
}

// Reference helpers
class Reference {
  static create(txid: string, vout: number): Buffer;
  static parse(refBytes: Buffer): { txid: string, vout: number };
  static buildPushRef(ref: Buffer): Script;
}

// Transaction introspection
class TxIntrospector {
  constructor(tx: Transaction, inputCoins: Coin[]);
  
  getInputValue(index: number): bigint;
  getInputScript(index: number): Script;
  getOutputValue(index: number): bigint;
  getOutputScript(index: number): Script;
  getRefSummary(): RefSummary;
}
```

##### Phase 4: Developer Tools Integration (2 weeks)
```typescript
// Export for radiant-deb
Transaction.prototype.toDebugContext(): DebugContext;
Transaction.prototype.toRadiantDebHex(): string;

// RadiantScript artifact support
import { Artifact } from '@radiantblockchain/radiantscript';
const contract = new Contract(artifact);
```

#### 3.3.3 Package Structure Proposal

```
@radiantblockchain/radiantjs (meta-package)
├── @radiantblockchain/core        # Primitives, crypto
├── @radiantblockchain/script      # Script, opcodes
├── @radiantblockchain/transaction # Transaction building
├── @radiantblockchain/contract    # Contract interaction
├── @radiantblockchain/wallet      # HD wallets, keys
└── @radiantblockchain/network     # P2P, Electrum client
```

---

### 3.4 ElectrumX-Core Upgrades (P2)

**Current State:** Stable indexer

**Location:** `/Downloads/ElectrumX-Core`

#### 3.4.1 Proposed Upgrades

##### Reference Indexing
```python
# New index for reference queries
class RefIndex:
    def get_utxos_by_ref(self, ref_bytes: bytes) -> List[UTXO]
    def get_ref_history(self, ref_bytes: bytes) -> List[TxRef]
    def get_code_script_hash_utxos(self, csh: bytes) -> List[UTXO]
```

##### Developer API Endpoints
```
# New RPC methods
blockchain.ref.get_utxos(ref_hex) -> [utxo, ...]
blockchain.ref.get_history(ref_hex) -> [tx_ref, ...]
blockchain.codescripthash.get_utxos(csh_hex) -> [utxo, ...]
blockchain.contract.get_state(contract_id) -> state_data
```

##### WebSocket Subscriptions
```
# Subscribe to reference updates
blockchain.ref.subscribe(ref_hex)
blockchain.codescripthash.subscribe(csh_hex)
```

---

## 4. Integration Points

### 4.1 Cross-Project Data Formats

#### 4.1.1 RadiantScript Artifact Format

```json
{
  "version": "2.0",
  "name": "FungibleToken",
  "compiler": {
    "name": "radc",
    "version": "0.2.0"
  },
  "abi": [
    {
      "name": "transfer",
      "inputs": [
        {"name": "sig", "type": "sig"},
        {"name": "pk", "type": "pubkey"},
        {"name": "amount", "type": "int"}
      ]
    }
  ],
  "bytecode": "76a914...",
  "source": "contract FungibleToken...",
  "sourceMap": {
    "0": {"line": 1, "col": 0},
    "3": {"line": 2, "col": 4}
  },
  "debug": {
    "opcodes": ["OP_DUP", "OP_HASH160", ...],
    "stackEffects": [[0, 1], [1, 1], ...]
  }
}
```

#### 4.1.2 radiant-deb Context Format

```json
{
  "version": "1.0",
  "network": "mainnet",
  "tx": {
    "hex": "0200000001...",
    "inputs": [
      {
        "txid": "abc123...",
        "vout": 0,
        "scriptSig": "483045...",
        "sequence": 4294967295
      }
    ],
    "outputs": [...]
  },
  "inputCoins": [
    {
      "txid": "def456...",
      "vout": 0,
      "scriptPubKey": "76a914...",
      "value": 100000000
    }
  ],
  "inputIndex": 0,
  "flags": 2147483647
}
```

### 4.2 Shared Constants

Create a shared constants package used by all projects:

```typescript
// @radiantblockchain/constants

export const Opcodes = {
  // Standard opcodes
  OP_0: 0x00,
  OP_CHECKSIG: 0xac,
  
  // Radiant-specific
  OP_INPUTINDEX: 0xc0,
  OP_PUSHINPUTREF: 0xd0,
  OP_STATESEPARATOR: 0xbd,
  // ...
} as const;

export const ScriptFlags = {
  SCRIPT_VERIFY_NONE: 0,
  SCRIPT_64_BIT_INTEGERS: 1 << 24,
  SCRIPT_NATIVE_INTROSPECTION: 1 << 25,
  SCRIPT_ENHANCED_REFERENCES: 1 << 26,
  // ...
} as const;

export const Limits = {
  MAX_SCRIPT_ELEMENT_SIZE: 32_000_000,
  MAX_SCRIPT_SIZE: 32_000_000,
  MAX_STACK_SIZE: 32_000_000,
  MAX_SCRIPTNUM_SIZE: 8,
} as const;
```

---

## 5. Unified Development Workflow

### 5.1 Smart Contract Development Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    DEVELOPMENT WORKFLOW                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. WRITE          2. COMPILE         3. DEBUG                  │
│  ┌──────────┐      ┌──────────┐      ┌──────────┐              │
│  │  .rad    │─────▶│   radc   │─────▶│radiant-  │              │
│  │  file    │      │          │      │   deb    │              │
│  └──────────┘      └──────────┘      └──────────┘              │
│       │                  │                 │                    │
│       ▼                  ▼                 ▼                    │
│  RadiantScript     Artifact.json     Step-by-step              │
│  high-level        with bytecode     execution                  │
│                                                                  │
│  4. TEST           5. DEPLOY         6. INTERACT               │
│  ┌──────────┐      ┌──────────┐      ┌──────────┐              │
│  │  Jest/   │─────▶│radiantjs │─────▶│ Wallet/  │              │
│  │  Mocha   │      │          │      │  dApp    │              │
│  └──────────┘      └──────────┘      └──────────┘              │
│       │                  │                 │                    │
│       ▼                  ▼                 ▼                    │
│  Unit tests        Broadcast tx      Live contract             │
│  with mocks        to network        interaction               │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 CLI Tool Integration

```bash
# Full development workflow

# 1. Create new contract project
npx create-radiant-contract my-token
cd my-token

# 2. Write contract (my-token.rad)
# 3. Compile
npx radc my-token.rad -o artifacts/my-token.json --debug

# 4. Debug in radiant-deb
radiant-deb --rxd \
  --artifact=artifacts/my-token.json \
  --function=transfer \
  --args='["<sig>", "<pubkey>", 1000]'

# 5. Run tests
npm test

# 6. Deploy to testnet
npx radiant-deploy --network=testnet --artifact=artifacts/my-token.json

# 7. Interact
npx radiant-call --contract=<address> --function=transfer --args='[...]'
```

---

## 6. Timeline & Milestones

### 6.1 Master Timeline (6 months)

```
Month 1-2: Foundation
├── Week 1-4:  radiant-deb Phase 1-2 (Foundation + Core Engine)
├── Week 5-6:  RadiantScript opcode audit
└── Week 7-8:  radiantjs TypeScript planning

Month 3-4: Core Features  
├── Week 9-12:  radiant-deb Phase 3-4 (Tx + Introspection)
├── Week 13-14: RadiantScript debugging integration
└── Week 15-16: radiantjs TypeScript migration

Month 5: Integration
├── Week 17-18: radiant-deb Phase 5 (References)
├── Week 19-20: Cross-project integration testing
└── ElectrumX reference indexing

Month 6: Polish & Release
├── Week 21-22: radiant-deb Phase 6 (Polish)
├── Week 23:    Documentation sprint
└── Week 24:    Release candidates, community testing
```

### 6.2 Release Schedule

| Date | Milestone | Deliverables |
|------|-----------|--------------|
| **Month 2** | radiant-deb Alpha | Basic opcodes, CLI working |
| **Month 3** | RadiantScript 0.3 | Source maps, improved types |
| **Month 4** | radiantjs 2.0-beta | TypeScript, new opcodes |
| **Month 5** | radiant-deb Beta | Full opcode support |
| **Month 6** | Ecosystem 1.0 | All tools stable, integrated |

---

## 7. Resource Requirements

### 7.1 Development Team

| Role | Projects | FTE |
|------|----------|-----|
| C++ Developer | radiant-deb | 1.0 |
| TypeScript Developer | RadiantScript, radiantjs | 1.0 |
| Python Developer | ElectrumX-Core | 0.5 |
| Technical Writer | All | 0.5 |
| QA Engineer | All | 0.5 |

### 7.2 Infrastructure

- CI/CD: GitHub Actions
- Package Registry: npm, PyPI
- Documentation: GitBook or Docusaurus
- Testing: Regtest network, testnet faucet

---

## 8. Risk Assessment

### 8.1 Technical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Consensus drift from Radiant-Core | High | Link to Radiant-Core library |
| Large scope creep | Medium | Strict PRD adherence |
| Cross-project incompatibility | Medium | Shared constants package |
| Performance issues with large scripts | Low | Benchmark early, optimize |

### 8.2 Project Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Resource constraints | High | Prioritize P0/P1 projects |
| Community adoption | Medium | Early alpha releases, feedback |
| Maintenance burden | Medium | Automated testing, CI/CD |

---

## 9. Success Metrics

### 9.1 Quantitative Metrics

| Metric | Target |
|--------|--------|
| radiant-deb opcode coverage | 100% |
| RadiantScript test coverage | >80% |
| radiantjs bundle size | <100KB |
| Documentation pages | >50 |
| Example contracts | >20 |
| GitHub stars (combined) | >500 |

### 9.2 Qualitative Metrics

- Developer satisfaction survey: >4/5 rating
- Time to first contract deployment: <1 hour
- Community-contributed examples: >10
- Third-party tool integrations: >3

---

## 10. Open Questions

1. **Monorepo vs Multi-repo**: Should RadiantScript and radiantjs share a monorepo?

2. **Versioning Strategy**: Semantic versioning or CalVer for coordinated releases?

3. **Backward Compatibility**: How long to support older API versions?

4. **Community Governance**: Process for accepting external contributions?

5. **Commercial Support**: Plans for enterprise support/consulting?

---

## Appendix A: Current Codebase Analysis

### RadiantScript Structure
```
packages/
├── cashc/        # Compiler core (needs rename to radc)
├── cashscript/   # Runtime library  
└── utils/        # Shared utilities
```

### radiantjs Structure
```
lib/
├── address/      # Address encoding
├── crypto/       # Cryptographic primitives
├── encoding/     # Serialization
├── networks/     # Network parameters
├── script/       # Script building (needs Radiant opcodes)
├── transaction/  # Transaction building
└── ...
```

### ElectrumX-Core Structure
```
electrumx/
├── lib/          # Core library
├── server/       # Server implementation
└── wallet/       # Wallet support (if any)
```

---

## Appendix B: Useful Commands

```bash
# Build all projects locally
./scripts/build-all.sh

# Run integration tests
./scripts/integration-test.sh

# Generate documentation
./scripts/gen-docs.sh

# Create release
./scripts/release.sh v1.0.0
```

---

**Document Revision History**

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-23 | Cascade | Initial ecosystem plan |
