// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_SCRIPT_H
#define RXD_SCRIPT_H

#include <cstdint>
#include <string>
#include <vector>

namespace rxd {

/**
 * Radiant opcode enumeration
 * Includes all Bitcoin-compatible opcodes plus Radiant-specific extensions
 * Reference: Radiant-Core src/script/script.h
 */
enum opcodetype : uint8_t {
    // Push value
    OP_0 = 0x00,
    OP_FALSE = OP_0,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2 = 0x4d,
    OP_PUSHDATA4 = 0x4e,
    OP_1NEGATE = 0x4f,
    OP_RESERVED = 0x50,
    OP_1 = 0x51,
    OP_TRUE = OP_1,
    OP_2 = 0x52,
    OP_3 = 0x53,
    OP_4 = 0x54,
    OP_5 = 0x55,
    OP_6 = 0x56,
    OP_7 = 0x57,
    OP_8 = 0x58,
    OP_9 = 0x59,
    OP_10 = 0x5a,
    OP_11 = 0x5b,
    OP_12 = 0x5c,
    OP_13 = 0x5d,
    OP_14 = 0x5e,
    OP_15 = 0x5f,
    OP_16 = 0x60,

    // Control
    OP_NOP = 0x61,
    OP_VER = 0x62,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_VERIF = 0x65,
    OP_VERNOTIF = 0x66,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
    OP_RETURN = 0x6a,

    // Stack operations
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_2DROP = 0x6d,
    OP_2DUP = 0x6e,
    OP_3DUP = 0x6f,
    OP_2OVER = 0x70,
    OP_2ROT = 0x71,
    OP_2SWAP = 0x72,
    OP_IFDUP = 0x73,
    OP_DEPTH = 0x74,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_NIP = 0x77,
    OP_OVER = 0x78,
    OP_PICK = 0x79,
    OP_ROLL = 0x7a,
    OP_ROT = 0x7b,
    OP_SWAP = 0x7c,
    OP_TUCK = 0x7d,

    // Splice operations (re-enabled in Radiant)
    OP_CAT = 0x7e,
    OP_SPLIT = 0x7f,
    OP_NUM2BIN = 0x80,
    OP_BIN2NUM = 0x81,
    OP_SIZE = 0x82,

    // Bit logic
    OP_INVERT = 0x83,
    OP_AND = 0x84,
    OP_OR = 0x85,
    OP_XOR = 0x86,
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,
    OP_RESERVED1 = 0x89,
    OP_RESERVED2 = 0x8a,

    // Numeric
    OP_1ADD = 0x8b,
    OP_1SUB = 0x8c,
    OP_2MUL = 0x8d,
    OP_2DIV = 0x8e,
    OP_NEGATE = 0x8f,
    OP_ABS = 0x90,
    OP_NOT = 0x91,
    OP_0NOTEQUAL = 0x92,
    OP_ADD = 0x93,
    OP_SUB = 0x94,
    OP_MUL = 0x95,
    OP_DIV = 0x96,
    OP_MOD = 0x97,
    OP_LSHIFT = 0x98,
    OP_RSHIFT = 0x99,
    OP_BOOLAND = 0x9a,
    OP_BOOLOR = 0x9b,
    OP_NUMEQUAL = 0x9c,
    OP_NUMEQUALVERIFY = 0x9d,
    OP_NUMNOTEQUAL = 0x9e,
    OP_LESSTHAN = 0x9f,
    OP_GREATERTHAN = 0xa0,
    OP_LESSTHANOREQUAL = 0xa1,
    OP_GREATERTHANOREQUAL = 0xa2,
    OP_MIN = 0xa3,
    OP_MAX = 0xa4,
    OP_WITHIN = 0xa5,

    // Crypto
    OP_RIPEMD160 = 0xa6,
    OP_SHA1 = 0xa7,
    OP_SHA256 = 0xa8,
    OP_HASH160 = 0xa9,
    OP_HASH256 = 0xaa,
    OP_CODESEPARATOR = 0xab,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,
    OP_CHECKMULTISIG = 0xae,
    OP_CHECKMULTISIGVERIFY = 0xaf,

    // Expansion
    OP_NOP1 = 0xb0,
    OP_CHECKLOCKTIMEVERIFY = 0xb1,
    OP_NOP2 = OP_CHECKLOCKTIMEVERIFY,
    OP_CHECKSEQUENCEVERIFY = 0xb2,
    OP_NOP3 = OP_CHECKSEQUENCEVERIFY,
    OP_NOP4 = 0xb3,
    OP_NOP5 = 0xb4,
    OP_NOP6 = 0xb5,
    OP_NOP7 = 0xb6,
    OP_NOP8 = 0xb7,
    OP_NOP9 = 0xb8,
    OP_NOP10 = 0xb9,

    // More crypto (BCH-derived)
    OP_CHECKDATASIG = 0xba,
    OP_CHECKDATASIGVERIFY = 0xbb,

    // Additional byte string operations
    OP_REVERSEBYTES = 0xbc,

    // ========================================
    // RADIANT-SPECIFIC OPCODES
    // ========================================

    // State separator (0xBD-0xBF)
    OP_STATESEPARATOR = 0xbd,
    OP_STATESEPARATORINDEX_UTXO = 0xbe,
    OP_STATESEPARATORINDEX_OUTPUT = 0xbf,

    // Native Introspection (0xC0-0xCD)
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

    // SHA512/256 functions (0xCE-0xCF)
    OP_SHA512_256 = 0xce,
    OP_HASH512_256 = 0xcf,

    // Reference opcodes (0xD0-0xED)
    OP_PUSHINPUTREF = 0xd0,
    OP_REQUIREINPUTREF = 0xd1,
    OP_DISALLOWPUSHINPUTREF = 0xd2,
    OP_DISALLOWPUSHINPUTREFSIBLING = 0xd3,
    OP_REFHASHDATASUMMARY_UTXO = 0xd4,
    OP_REFHASHVALUESUM_UTXOS = 0xd5,
    OP_REFHASHDATASUMMARY_OUTPUT = 0xd6,
    OP_REFHASHVALUESUM_OUTPUTS = 0xd7,
    OP_PUSHINPUTREFSINGLETON = 0xd8,
    OP_REFTYPE_UTXO = 0xd9,
    OP_REFTYPE_OUTPUT = 0xda,
    OP_REFVALUESUM_UTXOS = 0xdb,
    OP_REFVALUESUM_OUTPUTS = 0xdc,
    OP_REFOUTPUTCOUNT_UTXOS = 0xdd,
    OP_REFOUTPUTCOUNT_OUTPUTS = 0xde,
    OP_REFOUTPUTCOUNTZEROVALUED_UTXOS = 0xdf,
    OP_REFOUTPUTCOUNTZEROVALUED_OUTPUTS = 0xe0,
    OP_REFDATASUMMARY_UTXO = 0xe1,
    OP_REFDATASUMMARY_OUTPUT = 0xe2,
    OP_CODESCRIPTHASHVALUESUM_UTXOS = 0xe3,
    OP_CODESCRIPTHASHVALUESUM_OUTPUTS = 0xe4,
    OP_CODESCRIPTHASHOUTPUTCOUNT_UTXOS = 0xe5,
    OP_CODESCRIPTHASHOUTPUTCOUNT_OUTPUTS = 0xe6,
    OP_CODESCRIPTHASHZEROVALUEDOUTPUTCOUNT_UTXOS = 0xe7,
    OP_CODESCRIPTHASHZEROVALUEDOUTPUTCOUNT_OUTPUTS = 0xe8,
    OP_CODESCRIPTBYTECODE_UTXO = 0xe9,
    OP_CODESCRIPTBYTECODE_OUTPUT = 0xea,
    OP_STATESCRIPTBYTECODE_UTXO = 0xeb,
    OP_STATESCRIPTBYTECODE_OUTPUT = 0xec,
    OP_PUSH_TX_STATE = 0xed,

    // Invalid
    INVALIDOPCODE = 0xff,
};

/**
 * Get human-readable opcode name
 */
const char* GetOpName(opcodetype opcode);

/**
 * Check if opcode is a Radiant-specific opcode (not in Bitcoin)
 */
bool IsRadiantOpcode(opcodetype opcode);

/**
 * Check if opcode is a native introspection opcode
 */
bool IsIntrospectionOpcode(opcodetype opcode);

/**
 * Check if opcode is a reference opcode
 */
bool IsReferenceOpcode(opcodetype opcode);

/**
 * Check if opcode is a state separator opcode
 */
bool IsStateSeparatorOpcode(opcodetype opcode);

/**
 * Check if opcode is a push data opcode (0x00-0x4e)
 */
bool IsPushOpcode(opcodetype opcode);

/**
 * Check if opcode is disabled in Bitcoin but enabled in Radiant
 */
bool IsReenabledOpcode(opcodetype opcode);

/**
 * Parse opcode from string (e.g., "OP_CHECKSIG" -> 0xac)
 */
bool ParseOpcode(const std::string& str, opcodetype& opcode);

/**
 * Type alias for stack elements
 */
using valtype = std::vector<uint8_t>;

/**
 * Type alias for stack
 */
using StackT = std::vector<valtype>;

/**
 * CRxdScript - Radiant script wrapper
 * Extends functionality for Radiant-specific features
 */
class CRxdScript {
public:
    using const_iterator = std::vector<uint8_t>::const_iterator;
    
    CRxdScript() = default;
    CRxdScript(const std::vector<uint8_t>& data) : script_(data) {}
    CRxdScript(const uint8_t* begin, const uint8_t* end) : script_(begin, end) {}
    
    // Accessors
    const std::vector<uint8_t>& data() const { return script_; }
    size_t size() const { return script_.size(); }
    bool empty() const { return script_.empty(); }
    const_iterator begin() const { return script_.begin(); }
    const_iterator end() const { return script_.end(); }
    
    // Parse operations
    bool GetOp(const_iterator& pc, opcodetype& opcode, valtype& data) const;
    bool GetOp(const_iterator& pc, opcodetype& opcode) const;
    
    // Radiant-specific features
    bool HasStateSeparator() const;
    uint32_t GetStateSeparatorIndex() const;
    CRxdScript GetStateScript() const;
    CRxdScript GetCodeScript() const;
    
    // Script building
    CRxdScript& operator<<(opcodetype opcode);
    CRxdScript& operator<<(const valtype& data);
    CRxdScript& operator<<(int64_t n);
    
    // Serialization
    std::string ToHex() const;
    std::string ToAsm() const;
    static CRxdScript FromHex(const std::string& hex);
    static CRxdScript FromAsm(const std::string& asm_str);
    
    // Standard script checks
    bool IsPayToScriptHash() const;
    bool IsPayToPubKeyHash() const;
    bool IsPushOnly() const;
    bool IsUnspendable() const;
    
private:
    std::vector<uint8_t> script_;
};

} // namespace rxd

#endif // RXD_SCRIPT_H
