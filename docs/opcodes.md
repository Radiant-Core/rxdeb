# Opcodes

Opcodes are a central part of the script language in Radiant. Everything is an opcode, split into two types: push opcodes, which put something on the stack, and action opcodes, which do something, often manipulating the data on the stack.

In rxdeb, you can express opcodes in two ways:

* By opcode name, e.g. `OP_TRUE`, where you can often (but not always) skip the `OP_` prefix (i.e. `TRUE`)
* By opcode hex code, using the `OP_x` (or simply `x`) prefix, e.g. `OP_xc0` for `OP_INPUTINDEX`

## Radiant-Specific Opcodes

Radiant extends Bitcoin script with many new opcodes:

### Native Introspection (0xC0-0xCD)
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

### SHA512/256 Functions (0xCE-0xCF)
```
OP_SHA512_256 (0xCE)      - SHA512/256 hash
OP_HASH512_256 (0xCF)     - Double SHA512/256 hash
```

### Reference Opcodes (0xD0-0xED)
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

### State Management (0xBD-0xBF)
```
OP_STATESEPARATOR (0xBD)         - Separates state from code
OP_STATESEPARATORINDEX_UTXO (0xBE)
OP_STATESEPARATORINDEX_OUTPUT (0xBF)
```

### Re-enabled Opcodes (disabled in Bitcoin)
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

## Opcode Byte Map

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

## Note on SegWit/Taproot

Radiant does **not** support SegWit or Taproot. There are no witness programs or tapscript opcodes.
