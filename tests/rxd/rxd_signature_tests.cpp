// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../test/catch.hpp"
#include "../../rxd/rxd_signature.h"
#include "../../rxd/rxd_crypto.h"
#include "../../rxd/rxd_tx.h"

using namespace rxd;

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

} // anonymous namespace

TEST_CASE("Sighash type helpers", "[rxd][signature]") {
    SECTION("GetBaseSigHashType") {
        REQUIRE(GetBaseSigHashType(SIGHASH_ALL) == SIGHASH_ALL);
        REQUIRE(GetBaseSigHashType(SIGHASH_ALL_FORKID) == SIGHASH_ALL);
        REQUIRE(GetBaseSigHashType(SIGHASH_NONE_FORKID) == SIGHASH_NONE);
        REQUIRE(GetBaseSigHashType(SIGHASH_SINGLE_FORKID) == SIGHASH_SINGLE);
    }
    
    SECTION("HasForkId") {
        REQUIRE(HasForkId(SIGHASH_ALL) == false);
        REQUIRE(HasForkId(SIGHASH_ALL_FORKID) == true);
        REQUIRE(HasForkId(SIGHASH_NONE_FORKID) == true);
    }
    
    SECTION("HasAnyoneCanPay") {
        REQUIRE(HasAnyoneCanPay(SIGHASH_ALL) == false);
        REQUIRE(HasAnyoneCanPay(SIGHASH_ALL_ANYONECANPAY) == true);
    }
}

TEST_CASE("Signature encoding validation", "[rxd][signature]") {
    SECTION("Valid DER signature") {
        // Example valid DER signature (without sighash byte)
        // Structure: 30 [len] 02 [rlen] [R] 02 [slen] [S]
        // Note: R and S values must not have high bit set (would indicate negative)
        auto sig = HexToBytes(
            "3044"                                                          // SEQUENCE, 68 bytes
            "0220"                                                          // INTEGER, 32 bytes (R)
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
            "0220"                                                          // INTEGER, 32 bytes (S)
            "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"
        );
        REQUIRE(IsValidSignatureEncoding(sig) == true);
    }
    
    SECTION("Invalid signature - wrong prefix") {
        auto sig = HexToBytes("31060201010201ff");
        REQUIRE(IsValidSignatureEncoding(sig) == false);
    }
    
    SECTION("Invalid signature - too short") {
        auto sig = HexToBytes("3006020100020100");
        REQUIRE(IsValidSignatureEncoding(sig) == false);
    }
}

TEST_CASE("Public key validation", "[rxd][signature]") {
    SECTION("Valid compressed pubkey (02)") {
        auto pubkey = HexToBytes(
            "02"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        );
        REQUIRE(IsValidPubKey(pubkey) == true);
    }
    
    SECTION("Valid compressed pubkey (03)") {
        auto pubkey = HexToBytes(
            "03"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        );
        REQUIRE(IsValidPubKey(pubkey) == true);
    }
    
    SECTION("Valid uncompressed pubkey") {
        auto pubkey = HexToBytes(
            "04"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
            "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8"
        );
        REQUIRE(IsValidPubKey(pubkey) == true);
    }
    
    SECTION("Invalid pubkey - wrong prefix") {
        auto pubkey = HexToBytes(
            "05"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        );
        REQUIRE(IsValidPubKey(pubkey) == false);
    }
    
    SECTION("Invalid pubkey - wrong length") {
        auto pubkey = HexToBytes("02abcd");
        REQUIRE(IsValidPubKey(pubkey) == false);
    }
}

TEST_CASE("Sighash byte extraction", "[rxd][signature]") {
    SECTION("GetSigHashType") {
        std::vector<uint8_t> sig = {0x30, 0x44, 0x02, 0x20, 0x41};  // Sighash = 0x41
        REQUIRE(GetSigHashType(sig) == 0x41);
    }
    
    SECTION("StripSigHashType") {
        std::vector<uint8_t> sig = {0x30, 0x44, 0x02, 0x20, 0x41};
        auto stripped = StripSigHashType(sig);
        REQUIRE(stripped.size() == 4);
        REQUIRE(stripped.back() == 0x20);
    }
    
    SECTION("Empty signature") {
        std::vector<uint8_t> sig;
        REQUIRE(GetSigHashType(sig) == 0);
        REQUIRE(StripSigHashType(sig).empty());
    }
}

TEST_CASE("SignatureChecker", "[rxd][signature]") {
    SECTION("DummySignatureChecker always succeeds") {
        DummySignatureChecker checker;
        
        std::vector<uint8_t> sig = {0x30, 0x06, 0x41};
        std::vector<uint8_t> pubkey = HexToBytes(
            "02"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        );
        CRxdScript script;
        
        REQUIRE(checker.CheckSig(sig, pubkey, script) == true);
    }
    
    SECTION("DummySignatureChecker fails on empty inputs") {
        DummySignatureChecker checker;
        CRxdScript script;
        
        REQUIRE(checker.CheckSig({}, {0x02}, script) == false);
        REQUIRE(checker.CheckSig({0x30}, {}, script) == false);
    }
    
    SECTION("CheckLockTime always succeeds for dummy") {
        DummySignatureChecker checker;
        REQUIRE(checker.CheckLockTime(500000) == true);
    }
    
    SECTION("CheckSequence always succeeds for dummy") {
        DummySignatureChecker checker;
        REQUIRE(checker.CheckSequence(100) == true);
    }
}

TEST_CASE("SignatureHash computation", "[rxd][signature]") {
    SECTION("Empty transaction returns valid hash") {
        CRxdTx tx;
        CRxdScript script;
        
        // Should not crash, returns zeros for invalid input
        auto hash = SignatureHash(tx, 0, script, 0, SIGHASH_ALL_FORKID);
        REQUIRE(hash.size() == 32);
    }
    
    SECTION("Basic transaction sighash") {
        // Create a simple transaction
        CRxdTx tx;
        tx.SetVersion(2);
        tx.SetLockTime(0);
        
        // Add an input
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x11));
        input.SetPrevIndex(0);
        input.SetSequence(0xffffffff);
        tx.AddInput(input);
        
        // Add an output
        CRxdTxOut output;
        output.SetValue(100000);
        output.SetScript(CRxdScript());
        tx.AddOutput(output);
        
        // Create script code
        CRxdScript scriptCode;
        scriptCode << OP_DUP << OP_HASH160;
        
        // Compute sighash
        auto hash = SignatureHash(tx, 0, scriptCode, 200000, SIGHASH_ALL_FORKID);
        
        REQUIRE(hash.size() == 32);
        // Hash should be deterministic
        auto hash2 = SignatureHash(tx, 0, scriptCode, 200000, SIGHASH_ALL_FORKID);
        REQUIRE(hash == hash2);
    }
    
    SECTION("Different sighash types produce different hashes") {
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x22));
        input.SetPrevIndex(0);
        input.SetSequence(0xffffffff);
        tx.AddInput(input);
        
        CRxdTxOut output;
        output.SetValue(50000);
        tx.AddOutput(output);
        
        CRxdScript scriptCode;
        
        auto hashAll = SignatureHash(tx, 0, scriptCode, 100000, SIGHASH_ALL_FORKID);
        auto hashNone = SignatureHash(tx, 0, scriptCode, 100000, SIGHASH_NONE_FORKID);
        auto hashSingle = SignatureHash(tx, 0, scriptCode, 100000, SIGHASH_SINGLE_FORKID);
        
        REQUIRE(hashAll != hashNone);
        REQUIRE(hashAll != hashSingle);
        REQUIRE(hashNone != hashSingle);
    }
    
    SECTION("ANYONECANPAY produces different hash") {
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input1, input2;
        input1.SetPrevTxId(std::vector<uint8_t>(32, 0x33));
        input1.SetPrevIndex(0);
        input2.SetPrevTxId(std::vector<uint8_t>(32, 0x44));
        input2.SetPrevIndex(1);
        tx.AddInput(input1);
        tx.AddInput(input2);
        
        CRxdTxOut output;
        output.SetValue(75000);
        tx.AddOutput(output);
        
        CRxdScript scriptCode;
        
        auto hashNormal = SignatureHash(tx, 0, scriptCode, 100000, SIGHASH_ALL_FORKID);
        auto hashACP = SignatureHash(tx, 0, scriptCode, 100000, SIGHASH_ALL_ANYONECANPAY);
        
        REQUIRE(hashNormal != hashACP);
    }
}

TEST_CASE("Real signature checker", "[rxd][signature]") {
    SECTION("Rejects missing FORKID") {
        CRxdTx tx;
        tx.SetVersion(2);
        
        CRxdTxIn input;
        input.SetPrevTxId(std::vector<uint8_t>(32, 0x55));
        input.SetPrevIndex(0);
        tx.AddInput(input);
        
        SignatureChecker checker(tx, 0, 100000);
        
        // Signature without FORKID (ends with 0x01 = SIGHASH_ALL)
        std::vector<uint8_t> sig = {0x30, 0x06, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00, 0x01};
        std::vector<uint8_t> pubkey = HexToBytes(
            "02"
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798"
        );
        CRxdScript script;
        
        // Should fail because FORKID is required
        REQUIRE(checker.CheckSig(sig, pubkey, script) == false);
    }
}
