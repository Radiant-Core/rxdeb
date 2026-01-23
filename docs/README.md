# rxdeb Documentation

rxdeb is a Radiant Blockchain script debugger, forked from btcdeb. It provides a CLI-based debugger for stepping through Radiant scripts, inspecting stack states, and debugging smart contracts.

## Documentation Index

### Core Documentation

- **[rxdeb.md](rxdeb.md)** - Main debugger usage guide
- **[RADIANT_DEB_PRD.md](RADIANT_DEB_PRD.md)** - Product Requirements Document
- **[RADIANT_DEVTOOLS_ECOSYSTEM_PLAN.md](RADIANT_DEVTOOLS_ECOSYSTEM_PLAN.md)** - Ecosystem integration plan

### Reference

- **[opcodes.md](opcodes.md)** - Opcode usage and syntax
- **[mock-values.md](mock-values.md)** - Mock signatures/keys and inline operators

### Examples

- **[txs/](txs/)** - Example transaction data files
- **[example-multisig-invalid-order.txt](example-multisig-invalid-order.txt)** - Multisig debugging example

## Quick Start

```bash
# Build rxdeb
./autogen.sh
./configure --enable-rxd
make -j4

# Basic script debugging
./rxdeb '[OP_1 OP_2 OP_ADD OP_3 OP_EQUAL]'

# With transaction context
./rxdeb --tx=<amount>:<hex> '[<script>]' <sig> <pubkey>

# With Electrum integration
./rxdeb --electrum=electrum.radiant.ovh:50002 --txid=<txid> --vin=0

# Debug RadiantScript artifact
./rxdeb --artifact=Contract.json --tx=<hex>
```

## Key Features

- **Full Radiant opcode support** - 50+ new opcodes including introspection and references
- **64-bit integers** - Native support for large integer arithmetic
- **BIP143+FORKID sighash** - Correct signature verification
- **Electrum integration** - Fetch live UTXO context
- **RadiantScript debugging** - Source-level stepping with `.rad` files
- **REPL interface** - Interactive step-through debugging

## Radiant vs Bitcoin Script

Radiant extends Bitcoin script significantly:

| Feature | Radiant |
|---------|---------|
| SegWit/Taproot | **Not supported** |
| Native Introspection | OP_INPUTINDEX, OP_TXVERSION, etc. |
| Reference Opcodes | OP_PUSHINPUTREF, OP_REFVALUESUM_*, etc. |
| Re-enabled Opcodes | OP_CAT, OP_MUL, OP_DIV, OP_MOD |
| 64-bit Integers | Full support |
| 32MB Limits | Scripts, stack, elements |
