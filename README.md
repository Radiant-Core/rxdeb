# rxdeb - Radiant Script Debugger

A powerful script debugger for Radiant Blockchain, forked from [btcdeb](https://github.com/bitcoin-core/btcdeb).

rxdeb allows you to step through Radiant scripts, inspect stack states, debug smart contracts, and interact with live UTXOs via Electrum.

## Features

- **Full Radiant opcode support** - All 50+ Radiant-specific opcodes including V2 hard fork opcodes
- **V2 Hard Fork opcodes** - OP_BLAKE3, OP_K12 hash execution; OP_LSHIFT, OP_RSHIFT bitwise shifts; OP_2MUL, OP_2DIV arithmetic
- **Native introspection debugging** - Debug OP_INPUTINDEX, OP_TXVERSION, etc.
- **Reference opcode debugging** - Step through OP_PUSHINPUTREF and related opcodes
- **64-bit integer arithmetic** - Full support for Radiant's extended number range
- **Electrum integration** - Fetch live UTXOs for debugging (v1.0)
- **RadiantScript source maps** - Step through `.rad` source code
- **REPL interface** - Interactive debugging with history and tab completion

## Installation

### From Source

```bash
git clone --recursive https://github.com/radiantblockchain/rxdeb.git
cd rxdeb
./autogen.sh
./configure
make
sudo make install
```

### Dependencies

- C++17 compiler (GCC 7+ or Clang 5+)
- autoconf, automake, libtool
- libsecp256k1 (included as submodule)
- Radiant-Core (included as submodule for consensus accuracy)

## Quick Start

### Debug a simple script

```bash
rxdeb '[OP_1 OP_2 OP_ADD OP_3 OP_EQUAL]'
```

### Debug with transaction context

```bash
rxdeb --tx=<spending_tx_hex> --txin=<input_tx_hex>
```

### Debug with Electrum (live UTXOs)

```bash
rxdeb --electrum=electrum.radiant.ovh:50002 --txid=<txid> --vin=0
```

### Debug RadiantScript artifact

```bash
rxdeb --artifact=MyContract.json --function=transfer --args='["<sig>", "<pk>"]'
```

## REPL Commands

| Command | Description |
|---------|-------------|
| `step` / `s` | Execute one instruction |
| `continue` / `c` | Run to completion |
| `rewind` / `r` | Go back one instruction |
| `stack` | Print main stack |
| `altstack` | Print alt stack |
| `print` | Print script with current position |
| `source` | Show RadiantScript source (if available) |
| `refs` | Show reference tracking state |
| `context` | Show transaction context |
| `help` | Show all commands |

## Radiant-Specific Features

### Native Introspection

```bash
# Debug introspection opcodes
rxdeb '[OP_INPUTINDEX OP_0 OP_EQUAL]' --tx=<hex> --txin=<hex>

# The debugger shows introspection context:
rxdeb> context
  Input Index: 0
  TX Version: 2
  Input Count: 2
  Output Count: 1
  ...
```

### Reference Debugging

```bash
# Debug reference opcodes
rxdeb '[OP_PUSHINPUTREF <ref36> ...]' --tx=<hex> --txin=<hex>

# Show reference tracking:
rxdeb> refs
  Push Refs: [abc123...0000]
  Require Refs: []
  Singleton Refs: []
```

### Source-Level Debugging

```bash
# Compile with debug info
npx radc MyContract.rad -o MyContract.json --debug

# Debug with source mapping
rxdeb --artifact=MyContract.json --tx=<hex>

rxdeb> source
  MyContract.rad:15
    require(hash160(pk) == PKH);
         ^^^^^^^^^
```

## Building from btcdeb Fork

This project maintains btcdeb's Bitcoin support while adding Radiant as an additional backend. The Radiant adapter layer lives in `rxd/`:

```
rxdeb/
├── rxd/                    # Radiant adapter layer (~200KB)
│   ├── rxd_params.cpp/h    # Network params, limits, flags
│   ├── rxd_script.cpp/h    # Extended opcodes (300+)
│   ├── rxd_tx.cpp/h        # Transaction types, serialization
│   ├── rxd_vm_adapter.cpp/h # VM stepping with 50+ opcodes
│   ├── rxd_context.cpp/h   # Execution context for introspection
│   ├── rxd_electrum.cpp/h  # Electrum client for live UTXOs
│   ├── rxd_crypto.cpp/h    # Hash functions (SHA256, HASH160, etc.)
│   ├── rxd_signature.cpp/h # Signature verification, sighash
│   ├── rxd_artifact.cpp/h  # RadiantScript JSON artifact parser
│   ├── rxd_core_bridge.cpp/h # Radiant-Core integration bridge
│   └── rxd_repl.cpp/h      # REPL commands
├── deps/
│   └── radiant-core/       # Radiant-Core submodule
├── secp256k1/              # libsecp256k1 submodule
├── tests/rxd/              # Radiant test suite
│   ├── rxd_script_tests.cpp
│   ├── rxd_vm_tests.cpp
│   ├── rxd_signature_tests.cpp
│   └── rxd_integration_tests.cpp
└── examples/rxd/           # Example scripts
```

## Configuration

### Environment Variables

| Variable | Description |
|----------|-------------|
| `RXDEB_NETWORK` | Default network (mainnet/testnet/regtest) |
| `RXDEB_ELECTRUM` | Default Electrum server |
| `RXDEB_DEBUG` | Enable debug logging |

### Config File

`~/.rxdeb/config`:
```ini
network = mainnet
electrum = electrum.radiant.ovh:50002
```

## License

MIT License - See [LICENSE](LICENSE)

## Contributing

Contributions welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

## Related Projects

- [Radiant-Core](https://github.com/Radiant-Core/Radiant-Core) - Full node
- [RadiantScript](https://github.com/Radiant-Core/RadiantScript) - Smart contract compiler
- [radiantjs](https://github.com/Radiant-Core/radiantjs) - JavaScript SDK
- [@radiantblockchain/constants](https://github.com/Radiant-Core/radiantblockchain-constants) - Shared constants
