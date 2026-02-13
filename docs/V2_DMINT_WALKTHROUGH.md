# V2 dMint Contract Debugger Walkthrough

## Overview

This guide walks through debugging a V2 dMint (decentralized mint) contract using `rxdeb`. The V2 hard fork (block 410,000) introduces 6 new opcodes that enable **trustless on-chain PoW validation** for token minting.

## V2 Opcodes

| Opcode | Hex | Purpose |
|--------|-----|---------|
| OP_BLAKE3 | 0xee | Blake3 hash (PoW algorithm) |
| OP_K12 | 0xef | KangarooTwelve hash (PoW algorithm) |
| OP_LSHIFT | 0x98 | Bitwise left shift (DAA arithmetic) |
| OP_RSHIFT | 0x99 | Bitwise right shift (DAA arithmetic) |
| OP_2MUL | 0x8d | Multiply by 2 (DAA arithmetic) |
| OP_2DIV | 0x8e | Divide by 2 (DAA arithmetic) |

All opcodes are gated behind `SCRIPT_ENHANCED_REFERENCES` and activate at `radiantCore2UpgradeHeight`.

## Quick Start

```bash
# Run the V2 dMint walkthrough example
rxdeb -z --script-file=examples/rxd/v2_dmint_contract.txt

# Step through with 'step' command (or just press Enter)
rxdeb> step
rxdeb> step
# ... continue stepping to observe each opcode

# View stack at any point
rxdeb> stack

# Rewind to a previous step
rxdeb> rewind
```

## Walkthrough Sections

### Section 1: OP_BLAKE3 PoW Verification

The core V2 innovation: miners must solve `BLAKE3(preimage || nonce) < target` and the consensus layer validates it on-chain.

```
# preimage (contract state) is pushed
# nonce (miner solution) is pushed  
# OP_CAT concatenates them
# OP_BLAKE3 hashes → 32-byte result on stack
```

**Key insight:** In V1, the indexer validated PoW (trusted). In V2, the script itself validates PoW (trustless).

### Section 2: OP_K12 Alternative

Same pattern using KangarooTwelve. The algorithm is chosen at contract deployment:
- Algorithm ID 0x00 → OP_HASH256 (SHA256d, legacy)
- Algorithm ID 0x01 → OP_BLAKE3
- Algorithm ID 0x02 → OP_K12

### Section 3: DAA Shift Arithmetic

On-chain DAA (Difficulty Adjustment Algorithm) uses shift opcodes for efficient power-of-2 scaling:

```
target << 3   →  target * 8  (difficulty decrease)
target >> 2   →  target / 4  (difficulty increase)
```

The ASERT-lite DAA clamps adjustment to ±4 bits per block, preventing extreme swings.

### Section 4: OP_2MUL / OP_2DIV

Quick multiply/divide by 2 with safety guarantees:
- **OP_2MUL** rejects on overflow (input * 2 > INT64_MAX)
- **OP_2DIV** truncates toward zero: `7 / 2 = 3`, `-3 / 2 = -1`

### Section 5: Combined Pattern

Shows the complete V2 dMint flow: preimage → hash → target comparison.

## Real Contract Bytecode

The actual V2 dMint contract bytecode has three parts:

```
Part A (preimage building):
  5175c0c855797ea8597959797ea87e5a7a7e

PoW hash opcode (per algorithm):
  aa = OP_HASH256 (SHA256d)
  ee = OP_BLAKE3
  ef = OP_K12

Part B (target comparison + contract integrity):
  bc01147f77587f04...75686d7551
```

**Note:** The `aa` bytes in Part B are `OP_HASH256` for contract integrity checks, NOT the PoW hash. They remain unchanged regardless of the selected mining algorithm.

## Debugging a Live dMint Transaction

```bash
# Fetch and debug a real dMint mint transaction:
rxdeb --electrum=electrum.radiant.ovh:50002 \
      --txid=<mint_tx_id> --vin=0

# Or with raw hex:
rxdeb --tx=<spending_tx_hex> --txin=<input_tx_hex> --select=0
```

## DAA Modes

| ID | Mode | On-Chain Arithmetic |
|----|------|-------------------|
| 0x00 | Fixed | None (static difficulty) |
| 0x01 | Epoch | OP_MUL, OP_DIV |
| 0x02 | ASERT | OP_LSHIFT, OP_RSHIFT, OP_2MUL, OP_2DIV |
| 0x03 | LWMA | OP_MUL, OP_DIV |
| 0x04 | Schedule | Table lookup |

## Security Notes

- **S-1 (CRITICAL):** OP_BLAKE3 rejects inputs > 1024 bytes (DoS protection)
- **S-2 (HIGH):** OP_LSHIFT/OP_RSHIFT reject shifts exceeding input bit-length
- **OP_2MUL:** Rejects if result overflows INT64_MAX

## References

- Glyph v2 Whitepaper §11.2 (dMint Algorithms), §11.4.1 (DAA Bytecodes), Appendix E
- Radiant Core `src/script/interpreter.cpp` — opcode implementations
- `test/functional/feature_v2_hash_opcodes.py` — 26 consensus tests
