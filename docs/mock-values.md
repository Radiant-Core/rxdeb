# Mock values and inline operators

rxdeb supports mock values for signatures and keys, as well as inline operators.

## Mock values

Without providing additional information, rxdeb cannot verify signatures in scripts that you pass to it. The normal way to verify signatures is to provide the input transaction and the transaction spending it, or alternatively give the amount values in the input transaction, and the transaction spending it.

With mock values, you can simply tell rxdeb that the signature `X` and the public key `Y` together sign "anything". It will simply skip over the cryptographic check (including signature encoding checks).

Mock values are provided in the form of signature and pubkey *pairs*, written as `signature:pubkey`, using the `--pretend-valid` command line argument to `rxdeb`.

Multiple mock values can be written using comma separation (`sig1:pub1,sig2:pub2`, and so on).

### A real world example

As an example, you can use mock values to "pretend" that a provided signature check succeeds without providing full transaction context.

The signature and pubkey are the two values before the `OP_DUP`. We get:

> rxdeb '[OP_2 OP_DROP 30440220192b64bb3952c144ead43fdb5286007388785d81fd90ecedfba8bf7a5ea0a1ad02205891ceb16a793adad74497ee5f4a3eb42f6a6e4e1602b540f434e711a7d352141 02ab74ff5864c29ab400030afc05a7c141c4970707fef9dbb0ac24115867c987a5 OP_DUP OP_HASH160 ccbdfc0f1e5fc51e01d50c9ebc37ca7c323c05b5 OP_EQUALVERIFY OP_CHECKSIG]' --pretend-valid=30440220192b64bb3952c144ead43fdb5286007388785d81fd90ecedfba8bf7a5ea0a1ad02205891ceb16a793adad74497ee5f4a3eb42f6a6e4e1602b540f434e711a7d352141:02ab74ff5864c29ab400030afc05a7c141c4970707fef9dbb0ac24115867c987a5

And now rxdeb is satisfied, as our provided pair matches the one encountered.

### Using actual mock values

In reality, we can provide anything we want as signature and/or pubkey values. If we are making a new type of script, for example, the signatures and pubkey cryptographic checks are not interesting. We simply assume they're valid. There is, however, one problem: we need the hash-160 value of the pubkeys, unless we want to skip the first half of the signature check (dup hash160 equalverify). For now, we can simply calculate this using rxdeb itself (note: pub1 is the literal string "pub1" here):

```bash
$ rxdeb
rxdeb -- type `./rxdeb -h` for start up options
0 op script loaded. type `help` for usage information
script  |  stack
--------+--------
rxdeb> tf hash160 pub1
0c3071cf08289580e0e2030a388998460efd39e1
```

and then replace the above with this:

> rxdeb '[OP_2 OP_DROP sig1 pub1 OP_DUP OP_HASH160 0c3071cf08289580e0e2030a388998460efd39e1 OP_EQUALVERIFY OP_CHECKSIG]' --pretend-valid=sig1:pub1

However, we can use inline operators instead.

## Inline operators

Inline operators let you transform values on the fly as they are added to the stack. Operators take one single argument, and do not allow spaces. Operators which take multiple arguments must be given these as a script with each argument pushed to the stack internally (i.e. exactly the same way the rxdeb tf command works).

```bash
$ rxdeb '[sha256(0x1234)]'
rxdeb -- type `./rxdeb -h` for start up options
valid script
1 op script loaded. type `help` for usage information
script                                                           |  stack
-----------------------------------------------------------------+--------
3a103a4e5729ad68c02a678ae39accfbc0ae208096437401b7ceab63cca0622f |
#0000 3a103a4e5729ad68c02a678ae39accfbc0ae208096437401b7ceab63cca0622f
rxdeb>
```

The example in the section above can now be done without calculating the hash first by doing:

```bash
$ rxdeb '[OP_2 OP_DROP sig1 pub1 OP_DUP OP_HASH160 hash160(pub1) OP_EQUALVERIFY OP_CHECKSIG]' --pretend-valid=sig1:pub1
rxdeb 0.2.16 -- type `./rxdeb -h` for start up options
valid script
9 op script loaded. type `help` for usage information
script                                   |  stack
-----------------------------------------+--------
2                                        |
OP_DROP                                  |
73696731                                 |
70756231                                 |
OP_DUP                                   |
OP_HASH160                               |
0c3071cf08289580e0e2030a388998460efd39e1 |
OP_EQUALVERIFY                           |
OP_CHECKSIG                              |
#0000 2
rxdeb>
```

The following inline operators are defined (but mostly untested at this point in time):

* echo (identity function)
* hex
* int
* reverse
* sha256
* ripemd160
* hash256
* hash160
* base58chkenc
* base58chkdec
* bech32enc
* bech32dec
* verify_sig 
* combine_pubkeys
* tweak_pubkey
* addr_to_spk
* spk_to_addr
* add
* sub

If `--enable-dangerous` was set in `./configure` call when compiling, these additional operators are also available:

* combine_privkeys
* multiply_privkeys
* nnegate_privkey
* encode_wif
* decode_wif
* sign
* get_pubkey

These closely resemble the rxdeb tf command. For more information, type `tf -h` from inside rxdeb.
