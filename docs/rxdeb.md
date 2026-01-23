# rxdeb Script Debugger

The `rxdeb` command can step through a Radiant Script and show stack content and operations on a per-op level.

```
rxdeb> help
step     Execute one instruction and iterate in the script.
rewind   Go back in time one instruction.
stack    Print stack content.
altstack Print altstack content.
vfexec   Print vfexec content.
exec     Execute command.
tf       Transform a value using a given function.
print    Print script.
refs     Display reference tracking state.
context  Show execution context (introspection data).
source   Display RadiantScript source (if artifact loaded).
help     Show help information.
```

## Basic Usage

```Bash
$ rxdeb '[OP_DUP OP_HASH160 897c81ac37ae36f7bc5b91356cfb0138bfacb3c1 OP_EQUALVERIFY OP_CHECKSIG]' 3045022100c7d8e302908fdc601b125c2734de63ed3bf54353e13a835313c2a2aa5e8f21810220131fad73787989d7fbbdbbd8420674f56bdf61fed5dc2653c826a4789c68501141 03b05bdbdf395e495a61add92442071e32703518b8fca3fc34149db4b56c93be42
valid script
5 op script loaded. type `help` for usage information
script                                                             |                                                             stack
-------------------------------------------------------------------+-------------------------------------------------------------------
OP_DUP                                                             | 03b05bdbdf395e495a61add92442071e32703518b8fca3fc34149db4b56c93be42
OP_HASH160                                                         | 3045022100c7d8e302908fdc601b125c2734de63ed3bf54353e13a835313c2a...
897c81ac37ae36f7bc5b91356cfb0138bfacb3c1                           |
OP_EQUALVERIFY                                                     |
OP_CHECKSIG                                                        |
#0001 OP_DUP
rxdeb>
```

Note: This example will fail on the OP_CHECKSIG part without transaction context. See "Signature Checking" below.

## Radiant-Specific Features

### 64-bit Integer Arithmetic

Radiant supports 64-bit integers (vs Bitcoin's 32-bit):

```Bash
$ rxdeb '[21000000000000 1 OP_ADD 21000000000001 OP_EQUAL]'
```

### Introspection Opcodes

Debug scripts using native introspection:

```Bash
$ rxdeb --tx=<hex> '[OP_INPUTINDEX OP_0 OP_EQUAL]'
```

### Reference Opcodes

Track token references during execution:

```Bash
$ rxdeb --tx=<hex> '[OP_PUSHINPUTREF <36-byte-ref>]'
rxdeb> refs  # Display current reference state
```

### State Separator

Debug scripts with state/code separation:

```Bash
$ rxdeb '[<state-data> OP_STATESEPARATOR <code>]'
```

## Signature Checking

For OP_CHECKSIG to work, the debugger needs transaction context. Radiant uses BIP143-style sighash with SIGHASH_FORKID (0x41).

Pass the transaction using `--tx=amount1,amount2:hexdata`:

```Bash
$ rxdeb --tx=0.3315983:02000000013a9bad9b0bb8859bd22811be544269ee34f272c1185deb0761cb9aefcbfbfdc2000000006a47304402200cc8b0471a38edad2ff9f9799521b7d948054817793c980eaf3a6637ddfb939702201c1a801461d4c3cf4de4e7336454dba0dd70b89d71f221e991cb6a79df1a860d012102ce9f5972fe1473c9b6948949f676bbf7893a03c5b4420826711ef518ceefd8dcfeffffff0226f20b00000000001976a914d138551aa10d1f891ba02689390f32ce09b71c1788ac28b0ed01000000001976a914870c7d8085e1712539d8d78363865c42d2b5f75a88ac5b880800 '[OP_DUP OP_HASH160 1290b657a78e201967c22d8022b348bd5e23ce17 OP_EQUALVERIFY OP_CHECKSIG]' 304402200cc8b0471a38edad2ff9f9799521b7d948054817793c980eaf3a6637ddfb939702201c1a801461d4c3cf4de4e7336454dba0dd70b89d71f221e991cb6a79df1a860d41 02ce9f5972fe1473c9b6948949f676bbf7893a03c5b4420826711ef518ceefd8dc
```

You can also pass both the spending transaction (`--tx=`) and the input transaction (`--txin=`):

```Bash
$ rxdeb --tx=<spending-tx-hex> --txin=<input-tx-hex>
```

## Electrum Integration

Fetch transaction context directly from an Electrum server:

```Bash
$ rxdeb --electrum=electrum.radiant.ovh:50002 --txid=<txid> --vin=0
```

## RadiantScript Artifacts

Debug compiled RadiantScript contracts with source-level stepping:

```Bash
$ rxdeb --artifact=FungibleToken.json --tx=<hex>
rxdeb> step    # Shows corresponding .rad source line
rxdeb> source  # Display current position in source file
```

## Transforms

rxdeb has a built-in toolkit for transforming values. Type `tf -h` to see available operations.

```Bash
rxdeb> tf sha256 alice
2bd806c97f0e00af1a1fc3328fa763a9269723c8db8fac4f93af71db186d6e90
rxdeb> tf hash160 mypubkey
0c3071cf08289580e0e2030a388998460efd39e1
```

If compiled with `--enable-dangerous`, additional operations are available for private key manipulation:

```Bash
rxdeb> tf get-xpubkey sha256(alice)
9997a497d964fc1a62885b05a51166a65a90df00492c8d7cf61d6accf54803be
rxdeb> tf sign sha256(message) sha256(alice)
bd910c7f1340c525384c739bbde410e7d37701fdfacbec0a0d6d1d360381a4c01e85aee61eb30de834f111630b6c47803e0f17bf6397573f28508168531019ec
```

## Inline Operators

Use inline operators to transform values on the fly:

```Bash
$ rxdeb '[sig1 pub1 OP_DUP OP_HASH160 hash160(pub1) OP_EQUALVERIFY OP_CHECKSIG]' --pretend-valid=sig1:pub1
```

Available inline operators include:
- `sha256`, `hash256`, `ripemd160`, `hash160`
- `sha512_256` (Radiant-specific)
- `hex`, `int`, `reverse`
- `base58chkenc`, `base58chkdec`
- `addr_to_spk`, `spk_to_addr`
- `add`, `sub`

## Network Selection

```Bash
$ rxdeb --network=rxd-main    # Mainnet (default)
$ rxdeb --network=rxd-test    # Testnet
$ rxdeb --network=rxd-regtest # Regtest
```

## Key Differences from Bitcoin Script

| Feature | Bitcoin | Radiant |
|---------|---------|---------|
| Max element size | 520 bytes | 32 MB |
| Max script size | 10 KB | 32 MB |
| Integer size | 32-bit | 64-bit |
| SigHash | Various | FORKID required |
| SegWit/Taproot | Yes | No |
| Native Introspection | No | Yes |
| Reference Opcodes | No | Yes |
| OP_CAT, OP_MUL, etc. | Disabled | Enabled |
