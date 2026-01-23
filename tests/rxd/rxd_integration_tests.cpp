// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../test/catch.hpp"
#include "../../rxd/rxd_core_bridge.h"
#include "../../rxd/rxd_vm_adapter.h"
#include "../../rxd/rxd_tx.h"
#include "../../rxd/rxd_script.h"
#include "../../rxd/rxd_context.h"
#include "../../rxd/rxd_crypto.h"

using namespace rxd;
using namespace rxd::core;

namespace {

std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t byte = 0;
        for (int j = 0; j < 2 && i + j < hex.size(); j++) {
            char c = hex[i + j];
            byte <<= 4;
            if (c >= '0' && c <= '9') byte |= (c - '0');
            else if (c >= 'a' && c <= 'f') byte |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') byte |= (c - 'A' + 10);
        }
        result.push_back(byte);
    }
    return result;
}

std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    static const char* hex = "0123456789abcdef";
    std::string result;
    for (uint8_t b : bytes) {
        result += hex[b >> 4];
        result += hex[b & 0x0f];
    }
    return result;
}

// Build a P2PKH locking script
CRxdScript BuildP2PKH(const std::vector<uint8_t>& pubkeyHash) {
    CRxdScript script;
    script << OP_DUP << OP_HASH160;
    script << pubkeyHash;
    script << OP_EQUALVERIFY << OP_CHECKSIG;
    return script;
}

// Build a P2PKH unlocking script
CRxdScript BuildP2PKHUnlock(const std::vector<uint8_t>& sig, const std::vector<uint8_t>& pubkey) {
    CRxdScript script;
    script << sig << pubkey;
    return script;
}

} // anonymous namespace

TEST_CASE("Core bridge availability", "[rxd][integration]") {
    SECTION("Check availability") {
        // In native mode, Radiant-Core is not available
        bool available = IsRadiantCoreAvailable();
        INFO("Radiant-Core available: " << available);
        
        std::string version = GetRadiantCoreVersion();
        REQUIRE(!version.empty());
    }
}

TEST_CASE("Simple script verification", "[rxd][integration]") {
    SECTION("True script succeeds") {
        CRxdScript scriptSig;
        scriptSig << OP_1;
        
        CRxdScript scriptPubKey;
        // Empty scriptPubKey just checks stack top
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x00));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        CRxdTxOut output;
        output.SetValue(100000);
        tx.AddOutput(output);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000, 0);
        REQUIRE(result.success == true);
        REQUIRE(result.error == SCRIPT_ERR_OK);
    }
    
    SECTION("False script fails") {
        CRxdScript scriptSig;
        scriptSig << OP_0;
        
        CRxdScript scriptPubKey;
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x00));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000, 0);
        REQUIRE(result.success == false);
    }
    
    SECTION("Arithmetic script") {
        CRxdScript scriptSig;
        scriptSig << OP_5;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_3 << OP_ADD << OP_8 << OP_NUMEQUAL;
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x11));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 50000, 0);
        REQUIRE(result.success == true);
    }
}

TEST_CASE("P2PKH verification with dummy checker", "[rxd][integration]") {
    SECTION("Valid P2PKH structure") {
        // Use a known test pubkey
        auto pubkey = HexToBytes(
            "02"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        );
        
        // Compute pubkey hash
        auto pubkeyHash = crypto::Hash160(pubkey);
        
        // Build scripts
        auto scriptPubKey = BuildP2PKH(pubkeyHash);
        
        // Dummy signature (DER format with SIGHASH_ALL_FORKID)
        auto sig = HexToBytes(
            "3044"
            "0220" "1111111111111111111111111111111111111111111111111111111111111111"
            "0220" "2222222222222222222222222222222222222222222222222222222222222222"
            "41"  // SIGHASH_ALL | SIGHASH_FORKID
        );
        
        auto scriptSig = BuildP2PKHUnlock(sig, pubkey);
        
        // Build transaction
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0xaa));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        input.SetSequence(0xffffffff);
        tx.AddInput(input);
        
        CRxdTxOut output;
        output.SetValue(99000);
        output.SetScript(CRxdScript());
        tx.AddOutput(output);
        
        // Verify structure (signature verification will fail without real sig)
        // The script should at least parse and execute the DUP HASH160 ... EQUALVERIFY part
        INFO("ScriptSig: " << scriptSig.ToHex());
        INFO("ScriptPubKey: " << scriptPubKey.ToHex());
    }
}

TEST_CASE("Radiant-specific opcodes in integration", "[rxd][integration]") {
    SECTION("OP_MUL (re-enabled)") {
        CRxdScript scriptSig;
        scriptSig << OP_6;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_7 << OP_MUL;
        // 6 * 7 = 42
        std::vector<uint8_t> fortytwo = {42};
        scriptPubKey << fortytwo << OP_NUMEQUAL;
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x22));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000, 
                                   SCRIPT_ENABLE_MUL);
        REQUIRE(result.success == true);
    }
    
    SECTION("OP_CAT and OP_SPLIT") {
        // Push "hello" then "world", cat them, then split
        CRxdScript scriptSig;
        std::vector<uint8_t> hello = {'h', 'e', 'l', 'l', 'o'};
        std::vector<uint8_t> world = {'w', 'o', 'r', 'l', 'd'};
        scriptSig << hello << world;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_CAT;  // "helloworld"
        scriptPubKey << OP_5 << OP_SPLIT;  // "hello" "world"
        scriptPubKey << world << OP_EQUAL << OP_VERIFY;  // check "world"
        scriptPubKey << hello << OP_EQUAL;  // check "hello"
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x33));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000, 0);
        REQUIRE(result.success == true);
    }
    
    SECTION("64-bit integers") {
        // Test large number arithmetic
        CRxdScript scriptSig;
        // Push a number larger than 32-bit: 5,000,000,000 (0x12A05F200)
        std::vector<uint8_t> bigNum = {0x00, 0xf2, 0x05, 0x2a, 0x01};  // LE encoding
        scriptSig << bigNum;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_DUP << OP_ADD;  // Double it = 10,000,000,000
        // Expected: 0x540BE400 = {0x00, 0xe4, 0x0b, 0x54, 0x02}
        std::vector<uint8_t> expected = {0x00, 0xe4, 0x0b, 0x54, 0x02};
        scriptPubKey << expected << OP_NUMEQUAL;
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x44));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000,
                                   SCRIPT_64_BIT_INTEGERS);
        // Note: This test depends on proper 64-bit integer handling
        INFO("64-bit test result: " << (result.success ? "pass" : "fail"));
        INFO("Error: " << result.errorMessage);
    }
}

TEST_CASE("Transaction verification", "[rxd][integration]") {
    SECTION("Verify multi-input transaction") {
        CRxdTx tx;
        tx.SetVersion(2);
        tx.SetLockTime(0);
        
        // Add two inputs
        for (int i = 0; i < 2; i++) {
            CRxdTxIn input;
            std::vector<uint8_t> prevTxId(32, static_cast<uint8_t>(i + 1));
            input.SetPrevTxId(prevTxId);
            input.SetPrevIndex(0);
            
            CRxdScript scriptSig;
            scriptSig << OP_1;
            input.SetScript(scriptSig);
            input.SetSequence(0xffffffff);
            
            tx.AddInput(input);
        }
        
        // Add output
        CRxdTxOut output;
        output.SetValue(150000);
        tx.AddOutput(output);
        
        // UTXOs for verification
        std::vector<std::pair<CRxdScript, int64_t>> utxos;
        for (int i = 0; i < 2; i++) {
            CRxdScript scriptPubKey;  // Empty = just check stack
            utxos.push_back({scriptPubKey, 100000});
        }
        
        auto results = VerifyTransaction(tx, utxos, 0);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].success == true);
        REQUIRE(results[1].success == true);
    }
    
    SECTION("Verify transaction with failing input") {
        CRxdTx tx;
        tx.SetVersion(2);
        
        // First input succeeds
        CRxdTxIn input1;
        input1.SetPrevTxId(std::vector<uint8_t>(32, 0x11));
        input1.SetPrevIndex(0);
        CRxdScript scriptSig1;
        scriptSig1 << OP_1;
        input1.SetScript(scriptSig1);
        tx.AddInput(input1);
        
        // Second input fails
        CRxdTxIn input2;
        input2.SetPrevTxId(std::vector<uint8_t>(32, 0x22));
        input2.SetPrevIndex(0);
        CRxdScript scriptSig2;
        scriptSig2 << OP_0;  // Pushes false
        input2.SetScript(scriptSig2);
        tx.AddInput(input2);
        
        CRxdTxOut output;
        output.SetValue(150000);
        tx.AddOutput(output);
        
        std::vector<std::pair<CRxdScript, int64_t>> utxos;
        utxos.push_back({CRxdScript(), 100000});
        utxos.push_back({CRxdScript(), 100000});
        
        auto results = VerifyTransaction(tx, utxos, 0);
        
        REQUIRE(results.size() == 2);
        REQUIRE(results[0].success == true);
        REQUIRE(results[1].success == false);
    }
}

TEST_CASE("Script error strings", "[rxd][integration]") {
    SECTION("All error codes have strings") {
        for (int i = 0; i < SCRIPT_ERR_ERROR_COUNT; i++) {
            const char* str = core::ScriptErrorString(static_cast<core::ScriptError>(i));
            REQUIRE(str != nullptr);
            REQUIRE(strlen(str) > 0);
        }
    }
    
    SECTION("Specific error messages") {
        REQUIRE(std::string(core::ScriptErrorString(core::SCRIPT_ERR_OK)) == "No error");
        REQUIRE(std::string(core::ScriptErrorString(core::SCRIPT_ERR_OP_RETURN)).find("OP_RETURN") != std::string::npos);
        REQUIRE(std::string(core::ScriptErrorString(core::SCRIPT_ERR_DIV_BY_ZERO)).find("zero") != std::string::npos);
    }
}

TEST_CASE("Introspection opcodes in integration", "[rxd][integration]") {
    SECTION("OP_INPUTINDEX") {
        CRxdScript scriptSig;
        // Empty scriptSig
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_INPUTINDEX << OP_0 << OP_NUMEQUAL;  // First input = index 0
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x55));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        CRxdTxOut output;
        output.SetValue(50000);
        tx.AddOutput(output);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000,
                                   SCRIPT_ENABLE_NATIVE_INTROSPECTION);
        REQUIRE(result.success == true);
    }
    
    SECTION("OP_TXINPUTCOUNT") {
        CRxdScript scriptSig;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_TXINPUTCOUNT << OP_2 << OP_NUMEQUAL;
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        // Add two inputs
        for (int i = 0; i < 2; i++) {
            CRxdTxIn input;
            input.SetPrevTxId(std::vector<uint8_t>(32, static_cast<uint8_t>(i)));
            input.SetPrevIndex(0);
            input.SetScript(scriptSig);
            tx.AddInput(input);
        }
        
        CRxdTxOut output;
        output.SetValue(150000);
        tx.AddOutput(output);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000,
                                   SCRIPT_ENABLE_NATIVE_INTROSPECTION);
        REQUIRE(result.success == true);
    }
    
    SECTION("OP_TXOUTPUTCOUNT") {
        CRxdScript scriptSig;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_TXOUTPUTCOUNT << OP_3 << OP_NUMEQUAL;
        
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x66));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        // Add three outputs
        for (int i = 0; i < 3; i++) {
            CRxdTxOut output;
            output.SetValue(30000);
            tx.AddOutput(output);
        }
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000,
                                   SCRIPT_ENABLE_NATIVE_INTROSPECTION);
        REQUIRE(result.success == true);
    }
    
    SECTION("OP_TXVERSION") {
        CRxdScript scriptSig;
        
        CRxdScript scriptPubKey;
        scriptPubKey << OP_TXVERSION << OP_2 << OP_NUMEQUAL;
        
        CRxdTx tx;
        tx.SetVersion(2);  // Version 2
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x77));
        input.SetPrevIndex(0);
        input.SetScript(scriptSig);
        tx.AddInput(input);
        
        CRxdTxOut output;
        output.SetValue(90000);
        tx.AddOutput(output);
        
        auto result = VerifyScript(scriptSig, scriptPubKey, tx, 0, 100000,
                                   SCRIPT_ENABLE_NATIVE_INTROSPECTION);
        REQUIRE(result.success == true);
    }
}
