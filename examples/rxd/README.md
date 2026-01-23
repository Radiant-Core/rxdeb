# rxdeb Examples

This directory contains example scripts demonstrating how to use rxdeb for debugging Radiant smart contracts.

## Basic Examples

### 1. Simple P2PKH (`p2pkh.txt`)

Debug a standard Pay-to-Public-Key-Hash script:

```bash
rxdeb --script-file=examples/rxd/p2pkh.txt
```

### 2. Arithmetic Operations (`arithmetic.txt`)

Demonstrates 64-bit arithmetic operations:

```bash
rxdeb --script-file=examples/rxd/arithmetic.txt
```

## Introspection Examples

### 3. Transaction Introspection (`introspection.txt`)

Demonstrates native introspection opcodes with transaction context:

```bash
rxdeb --script-file=examples/rxd/introspection.txt \
      --tx=<spending_tx_hex> \
      --txin=<input_tx_hex>
```

## Token Examples

### 4. Fungible Token (`fungible_token.txt`)

Debug a fungible token contract:

```bash
rxdeb --script-file=examples/rxd/fungible_token.txt
```

## Running Examples

### Interactive Mode

```bash
# Start interactive debugging
rxdeb --script-file=examples/rxd/p2pkh.txt

# REPL commands:
rxdeb> step          # Execute one opcode
rxdeb> stack         # Show stack
rxdeb> continue      # Run to completion
rxdeb> rewind        # Go back one step
rxdeb> print         # Show script with position
```

### Batch Mode

```bash
# Run script and show final result
rxdeb --script-file=examples/rxd/arithmetic.txt --quiet

# Pipe output
echo "OP_1 OP_2 OP_ADD" | rxdeb
```

### With Electrum

```bash
# Fetch live UTXO and debug
rxdeb --electrum=electrum.radiant.ovh:50002 \
      --address=1ABC... \
      --latest
```

## Creating Your Own Examples

1. Create a script file with one of these formats:
   - Assembly: `OP_1 OP_2 OP_ADD`
   - Hex: `515293`
   - JSON artifact from RadiantScript

2. For transaction context, provide either:
   - `--tx` and `--txin` hex
   - `--context` JSON file
   - `--electrum` server connection

3. Run with rxdeb and step through!
