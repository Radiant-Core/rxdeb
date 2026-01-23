// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_PARAMS_H
#define RXD_PARAMS_H

#include <cstdint>
#include <string>
#include <vector>

namespace rxd {

/**
 * Radiant network types
 */
enum class Network {
    MAINNET,
    TESTNET,
    REGTEST
};

/**
 * Network-specific chain parameters
 */
struct ChainParams {
    std::string name;
    uint8_t pubkeyPrefix;
    uint8_t scriptPrefix;
    uint8_t privateKeyPrefix;
    uint32_t magicBytes;
    uint16_t defaultPort;
    uint16_t defaultElectrumPort;
    std::vector<std::string> electrumServers;
    
    static const ChainParams& Mainnet();
    static const ChainParams& Testnet();
    static const ChainParams& Regtest();
    static const ChainParams& Get(Network network);
};

/**
 * Script limits - Radiant-specific values
 * Reference: Radiant-Core src/script/script.h
 */
namespace Limits {
    // Maximum number of bytes pushable to the stack (legacy Bitcoin)
    constexpr uint32_t MAX_SCRIPT_ELEMENT_SIZE_LEGACY = 520;
    
    // Maximum number of bytes pushable to the stack (Radiant)
    constexpr uint32_t MAX_SCRIPT_ELEMENT_SIZE = 32000000;
    
    // Maximum number of non-push operations per script
    constexpr uint32_t MAX_OPS_PER_SCRIPT = 32000000;
    
    // Maximum number of public keys per multisig
    constexpr uint32_t MAX_PUBKEYS_PER_MULTISIG = 20;
    
    // Maximum script length in bytes
    constexpr uint32_t MAX_SCRIPT_SIZE = 32000000;
    
    // Maximum number of values on script interpreter stack
    constexpr uint32_t MAX_STACK_SIZE = 32000000;
    
    // Maximum size for CScriptNum (64-bit in Radiant)
    constexpr size_t MAX_SCRIPTNUM_SIZE = 8;
    
    // Threshold for nLockTime interpretation
    constexpr uint32_t LOCKTIME_THRESHOLD = 500000000;
    
    // Reference size (36 bytes: 32-byte txid + 4-byte vout)
    constexpr size_t REF_SIZE = 36;
}

/**
 * Script verification flags
 * Reference: Radiant-Core src/script/script_flags.h
 */
namespace ScriptFlags {
    constexpr uint32_t SCRIPT_VERIFY_NONE = 0;
    constexpr uint32_t SCRIPT_VERIFY_P2SH = (1U << 0);
    constexpr uint32_t SCRIPT_VERIFY_STRICTENC = (1U << 1);
    constexpr uint32_t SCRIPT_VERIFY_DERSIG = (1U << 2);
    constexpr uint32_t SCRIPT_VERIFY_LOW_S = (1U << 3);
    constexpr uint32_t SCRIPT_VERIFY_SIGPUSHONLY = (1U << 5);
    constexpr uint32_t SCRIPT_VERIFY_MINIMALDATA = (1U << 6);
    constexpr uint32_t SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS = (1U << 7);
    constexpr uint32_t SCRIPT_VERIFY_CLEANSTACK = (1U << 8);
    constexpr uint32_t SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9);
    constexpr uint32_t SCRIPT_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10);
    constexpr uint32_t SCRIPT_VERIFY_MINIMALIF = (1U << 13);
    constexpr uint32_t SCRIPT_VERIFY_NULLFAIL = (1U << 14);
    constexpr uint32_t SCRIPT_ENABLE_SIGHASH_FORKID = (1U << 16);
    constexpr uint32_t SCRIPT_DISALLOW_SEGWIT_RECOVERY = (1U << 20);
    constexpr uint32_t SCRIPT_ENABLE_SCHNORR_MULTISIG = (1U << 21);
    constexpr uint32_t SCRIPT_VERIFY_INPUT_SIGCHECKS = (1U << 22);
    constexpr uint32_t SCRIPT_ENFORCE_SIGCHECKS = (1U << 23);
    constexpr uint32_t SCRIPT_64_BIT_INTEGERS = (1U << 24);
    constexpr uint32_t SCRIPT_NATIVE_INTROSPECTION = (1U << 25);
    constexpr uint32_t SCRIPT_ENHANCED_REFERENCES = (1U << 26);
    constexpr uint32_t SCRIPT_PUSH_TX_STATE = (1U << 27);
    
    // Standard flags for Radiant mainnet
    constexpr uint32_t STANDARD_SCRIPT_VERIFY_FLAGS = 
        SCRIPT_VERIFY_P2SH |
        SCRIPT_VERIFY_STRICTENC |
        SCRIPT_VERIFY_DERSIG |
        SCRIPT_VERIFY_LOW_S |
        SCRIPT_VERIFY_SIGPUSHONLY |
        SCRIPT_VERIFY_MINIMALDATA |
        SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS |
        SCRIPT_VERIFY_CLEANSTACK |
        SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY |
        SCRIPT_VERIFY_CHECKSEQUENCEVERIFY |
        SCRIPT_VERIFY_MINIMALIF |
        SCRIPT_VERIFY_NULLFAIL |
        SCRIPT_ENABLE_SIGHASH_FORKID |
        SCRIPT_64_BIT_INTEGERS |
        SCRIPT_NATIVE_INTROSPECTION |
        SCRIPT_ENHANCED_REFERENCES;
    
    // Mandatory flags (consensus)
    constexpr uint32_t MANDATORY_SCRIPT_VERIFY_FLAGS =
        SCRIPT_VERIFY_P2SH |
        SCRIPT_ENABLE_SIGHASH_FORKID;
}

/**
 * Sighash types
 */
namespace SigHash {
    constexpr uint32_t ALL = 0x01;
    constexpr uint32_t NONE = 0x02;
    constexpr uint32_t SINGLE = 0x03;
    constexpr uint32_t FORKID = 0x40;
    constexpr uint32_t ANYONECANPAY = 0x80;
    
    // Default sighash for Radiant
    constexpr uint32_t DEFAULT = ALL | FORKID;
}

/**
 * Get human-readable network name
 */
std::string NetworkName(Network network);

/**
 * Parse network from string
 */
Network ParseNetwork(const std::string& name);

} // namespace rxd

#endif // RXD_PARAMS_H
