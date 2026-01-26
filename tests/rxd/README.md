# Radiant Script Tests

This directory contains test vectors and test cases specific to Radiant blockchain script validation.

## Directory Structure

```
tests/rxd/
├── README.md                   # This file
├── rxd_script_tests.cpp        # Opcode-level tests
├── rxd_vm_tests.cpp            # VM stepping tests
├── rxd_introspection_tests.cpp # Introspection opcode tests
├── rxd_reference_tests.cpp     # Reference opcode tests
├── rxd_sighash_tests.cpp       # Signature hash tests
└── data/
    ├── valid_scripts.json      # Valid script test vectors
    ├── invalid_scripts.json    # Invalid script test vectors
    ├── valid_txs.json          # Valid transaction test vectors
    ├── invalid_txs.json        # Invalid transaction test vectors
    ├── introspection.json      # Introspection test cases
    └── references.json         # Reference opcode test cases
```

## Test Categories

### 1. Script Tests (`rxd_script_tests.cpp`)

Tests for individual opcode behavior:

- **Basic opcodes**: Push, stack ops, arithmetic
- **Re-enabled opcodes**: OP_CAT, OP_SPLIT, OP_MUL, OP_DIV, etc.
- **64-bit arithmetic**: Large number operations
- **Crypto opcodes**: CHECKSIG, CHECKMULTISIG, CHECKDATASIG

### 2. VM Tests (`rxd_vm_tests.cpp`)

Tests for VM stepping and state tracking:

- Step-by-step execution
- Stack state after each opcode
- Rewind functionality
- Error state handling

### 3. Introspection Tests (`rxd_introspection_tests.cpp`)

Tests for native introspection opcodes:

- OP_INPUTINDEX, OP_TXVERSION, etc.
- OP_UTXOVALUE, OP_UTXOBYTECODE
- OP_OUTPUTVALUE, OP_OUTPUTBYTECODE
- Context validation

### 4. Reference Tests (`rxd_reference_tests.cpp`)

Tests for reference opcodes:

- OP_PUSHINPUTREF
- OP_REQUIREINPUTREF
- Reference tracking and validation
- Value sum and count queries

## Test Data Format

### scripts.json

```json
[
  {
    "description": "Human-readable test description",
    "script_asm": "OP_1 OP_2 OP_ADD OP_3 OP_EQUAL",
    "script_hex": "515293510087",
    "initial_stack": [],
    "expected_final_stack": ["01"],
    "expected_result": true,
    "expected_error": null,
    "flags": ["SCRIPT_64_BIT_INTEGERS"]
  }
]
```

### transactions.json

```json
[
  {
    "description": "Basic P2PKH spend",
    "tx_hex": "0200000001...",
    "input_index": 0,
    "input_coins": [
      {
        "value": 100000000,
        "scriptPubKey": "76a914...88ac"
      }
    ],
    "expected_result": true,
    "expected_error": null
  }
]
```

### introspection.json

```json
[
  {
    "description": "OP_INPUTINDEX returns correct index",
    "script_asm": "OP_INPUTINDEX OP_0 OP_NUMEQUAL",
    "tx_hex": "...",
    "input_index": 0,
    "input_coins": [...],
    "expected_result": true
  }
]
```

## Running Tests

### Run all Radiant tests

```bash
make check-rxd
```

### Run specific test file

```bash
./rxdeb-test --gtest_filter="RxdScript*"
./rxdeb-test --gtest_filter="RxdIntrospection*"
./rxdeb-test --gtest_filter="RxdReference*"
```

### Generate golden tests from Radiant-Core

```bash
python3 scripts/generate_golden_tests.py \
    --node=localhost:7332 \
    --output=tests/rxd/data/
```

## Adding New Tests

1. Add test vector to appropriate JSON file in `data/`
2. Add C++ test case that loads and runs the vector
3. Document the test case in this README
4. Ensure test passes against Radiant-Core for consensus accuracy

## Consensus Accuracy

All tests should produce identical results to Radiant-Core's script interpreter.
When in doubt, generate golden tests from a running Radiant node.

```bash
# Compare rxdeb output to radiant-cli
./rxdeb --script="OP_1 OP_2 OP_ADD" | diff - <(radiant-cli decodescript ...)
```

## Test Coverage Goals

| Category | Target Coverage |
|----------|-----------------|
| Standard opcodes | 100% |
| Re-enabled opcodes | 100% |
| Introspection opcodes | 100% |
| Reference opcodes | 100% |
| Error cases | >90% |
| Edge cases | >80% |
