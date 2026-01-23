// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../test/catch.hpp"
#include "../../rxd/rxd_script.h"
#include "../../rxd/rxd_params.h"

using namespace rxd;

TEST_CASE("Opcode names are correct", "[rxd][script]") {
    SECTION("Standard opcodes") {
        REQUIRE(std::string(GetOpName(OP_0)) == "OP_0");
        REQUIRE(std::string(GetOpName(OP_1)) == "OP_1");
        REQUIRE(std::string(GetOpName(OP_16)) == "OP_16");
        REQUIRE(std::string(GetOpName(OP_DUP)) == "OP_DUP");
        REQUIRE(std::string(GetOpName(OP_HASH160)) == "OP_HASH160");
        REQUIRE(std::string(GetOpName(OP_CHECKSIG)) == "OP_CHECKSIG");
    }
    
    SECTION("Radiant-specific opcodes") {
        REQUIRE(std::string(GetOpName(OP_STATESEPARATOR)) == "OP_STATESEPARATOR");
        REQUIRE(std::string(GetOpName(OP_INPUTINDEX)) == "OP_INPUTINDEX");
        REQUIRE(std::string(GetOpName(OP_TXVERSION)) == "OP_TXVERSION");
        REQUIRE(std::string(GetOpName(OP_UTXOVALUE)) == "OP_UTXOVALUE");
        REQUIRE(std::string(GetOpName(OP_PUSHINPUTREF)) == "OP_PUSHINPUTREF");
        REQUIRE(std::string(GetOpName(OP_SHA512_256)) == "OP_SHA512_256");
    }
    
    SECTION("Re-enabled opcodes") {
        REQUIRE(std::string(GetOpName(OP_CAT)) == "OP_CAT");
        REQUIRE(std::string(GetOpName(OP_SPLIT)) == "OP_SPLIT");
        REQUIRE(std::string(GetOpName(OP_MUL)) == "OP_MUL");
        REQUIRE(std::string(GetOpName(OP_DIV)) == "OP_DIV");
    }
}

TEST_CASE("Opcode classification", "[rxd][script]") {
    SECTION("IsRadiantOpcode") {
        REQUIRE(IsRadiantOpcode(OP_STATESEPARATOR) == true);
        REQUIRE(IsRadiantOpcode(OP_INPUTINDEX) == true);
        REQUIRE(IsRadiantOpcode(OP_PUSHINPUTREF) == true);
        REQUIRE(IsRadiantOpcode(OP_DUP) == false);
        REQUIRE(IsRadiantOpcode(OP_ADD) == false);
    }
    
    SECTION("IsIntrospectionOpcode") {
        REQUIRE(IsIntrospectionOpcode(OP_INPUTINDEX) == true);
        REQUIRE(IsIntrospectionOpcode(OP_TXVERSION) == true);
        REQUIRE(IsIntrospectionOpcode(OP_UTXOVALUE) == true);
        REQUIRE(IsIntrospectionOpcode(OP_OUTPUTBYTECODE) == true);
        REQUIRE(IsIntrospectionOpcode(OP_DUP) == false);
    }
    
    SECTION("IsReferenceOpcode") {
        REQUIRE(IsReferenceOpcode(OP_PUSHINPUTREF) == true);
        REQUIRE(IsReferenceOpcode(OP_REQUIREINPUTREF) == true);
        REQUIRE(IsReferenceOpcode(OP_PUSHINPUTREFSINGLETON) == true);
        REQUIRE(IsReferenceOpcode(OP_DUP) == false);
    }
    
    SECTION("IsReenabledOpcode") {
        REQUIRE(IsReenabledOpcode(OP_CAT) == true);
        REQUIRE(IsReenabledOpcode(OP_SPLIT) == true);
        REQUIRE(IsReenabledOpcode(OP_MUL) == true);
        REQUIRE(IsReenabledOpcode(OP_DIV) == true);
        REQUIRE(IsReenabledOpcode(OP_ADD) == false);
    }
}

TEST_CASE("Opcode parsing", "[rxd][script]") {
    SECTION("Parse standard opcodes") {
        opcodetype op;
        REQUIRE(ParseOpcode("OP_DUP", op) == true);
        REQUIRE(op == OP_DUP);
        
        REQUIRE(ParseOpcode("OP_HASH160", op) == true);
        REQUIRE(op == OP_HASH160);
        
        REQUIRE(ParseOpcode("OP_CHECKSIG", op) == true);
        REQUIRE(op == OP_CHECKSIG);
    }
    
    SECTION("Parse Radiant opcodes") {
        opcodetype op;
        REQUIRE(ParseOpcode("OP_STATESEPARATOR", op) == true);
        REQUIRE(op == OP_STATESEPARATOR);
        
        REQUIRE(ParseOpcode("OP_INPUTINDEX", op) == true);
        REQUIRE(op == OP_INPUTINDEX);
        
        REQUIRE(ParseOpcode("OP_PUSHINPUTREF", op) == true);
        REQUIRE(op == OP_PUSHINPUTREF);
    }
    
    SECTION("Invalid opcode") {
        opcodetype op;
        REQUIRE(ParseOpcode("OP_INVALID_FAKE", op) == false);
        REQUIRE(ParseOpcode("", op) == false);
    }
}

TEST_CASE("CRxdScript basic operations", "[rxd][script]") {
    SECTION("Empty script") {
        CRxdScript script;
        REQUIRE(script.size() == 0);
        REQUIRE(script.empty() == true);
    }
    
    SECTION("Push opcode") {
        CRxdScript script;
        script << OP_DUP;
        REQUIRE(script.size() == 1);
        REQUIRE(script.data()[0] == OP_DUP);
    }
    
    SECTION("Push data") {
        CRxdScript script;
        valtype data = {0x01, 0x02, 0x03};
        script << data;
        REQUIRE(script.size() == 4);  // 1 byte length + 3 bytes data
    }
    
    SECTION("Script from bytes") {
        std::vector<uint8_t> bytes = {OP_DUP, OP_HASH160};
        CRxdScript script(bytes);
        REQUIRE(script.size() == 2);
    }
}

TEST_CASE("CRxdScript GetOp", "[rxd][script]") {
    SECTION("Read opcodes") {
        std::vector<uint8_t> bytes = {OP_DUP, OP_HASH160, OP_EQUAL};
        CRxdScript script(bytes);
        
        auto it = script.begin();
        opcodetype opcode;
        
        REQUIRE(script.GetOp(it, opcode) == true);
        REQUIRE(opcode == OP_DUP);
        
        REQUIRE(script.GetOp(it, opcode) == true);
        REQUIRE(opcode == OP_HASH160);
        
        REQUIRE(script.GetOp(it, opcode) == true);
        REQUIRE(opcode == OP_EQUAL);
        
        REQUIRE(script.GetOp(it, opcode) == false);  // End of script
    }
    
    SECTION("Read push data") {
        std::vector<uint8_t> bytes = {0x03, 0xaa, 0xbb, 0xcc, OP_DROP};
        CRxdScript script(bytes);
        
        auto it = script.begin();
        opcodetype opcode;
        valtype data;
        
        REQUIRE(script.GetOp(it, opcode, data) == true);
        REQUIRE(opcode == 0x03);
        REQUIRE(data.size() == 3);
        REQUIRE(data[0] == 0xaa);
        
        REQUIRE(script.GetOp(it, opcode, data) == true);
        REQUIRE(opcode == OP_DROP);
        REQUIRE(data.empty() == true);
    }
}

TEST_CASE("CRxdScript state separator", "[rxd][script]") {
    SECTION("Script without state separator") {
        std::vector<uint8_t> bytes = {OP_DUP, OP_HASH160};
        CRxdScript script(bytes);
        REQUIRE(script.HasStateSeparator() == false);
    }
    
    SECTION("Script with state separator") {
        std::vector<uint8_t> bytes = {OP_DUP, OP_STATESEPARATOR, OP_HASH160};
        CRxdScript script(bytes);
        REQUIRE(script.HasStateSeparator() == true);
        REQUIRE(script.GetStateSeparatorIndex() == 1);
    }
}

TEST_CASE("CRxdScript pattern detection", "[rxd][script]") {
    SECTION("IsPayToScriptHash") {
        // P2SH: OP_HASH160 <20 bytes> OP_EQUAL
        std::vector<uint8_t> p2sh = {OP_HASH160, 0x14};
        p2sh.insert(p2sh.end(), 20, 0x00);  // 20 zero bytes
        p2sh.push_back(OP_EQUAL);
        
        CRxdScript script(p2sh);
        REQUIRE(script.IsPayToScriptHash() == true);
    }
    
    SECTION("IsPayToPubKeyHash") {
        // P2PKH: OP_DUP OP_HASH160 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG
        std::vector<uint8_t> p2pkh = {OP_DUP, OP_HASH160, 0x14};
        p2pkh.insert(p2pkh.end(), 20, 0x00);  // 20 zero bytes
        p2pkh.push_back(OP_EQUALVERIFY);
        p2pkh.push_back(OP_CHECKSIG);
        
        CRxdScript script(p2pkh);
        REQUIRE(script.IsPayToPubKeyHash() == true);
    }
    
    SECTION("IsUnspendable") {
        std::vector<uint8_t> unspendable = {OP_RETURN, 0x04, 0x01, 0x02, 0x03, 0x04};
        CRxdScript script(unspendable);
        REQUIRE(script.IsUnspendable() == true);
    }
    
    SECTION("IsPushOnly") {
        std::vector<uint8_t> pushOnly = {OP_1, 0x02, 0xaa, 0xbb, OP_3};
        CRxdScript script(pushOnly);
        REQUIRE(script.IsPushOnly() == true);
        
        std::vector<uint8_t> notPushOnly = {OP_1, OP_DUP};
        CRxdScript script2(notPushOnly);
        REQUIRE(script2.IsPushOnly() == false);
    }
}

TEST_CASE("CRxdScript serialization", "[rxd][script]") {
    SECTION("ToHex") {
        std::vector<uint8_t> bytes = {OP_DUP, OP_HASH160};
        CRxdScript script(bytes);
        REQUIRE(script.ToHex() == "76a9");
    }
    
    SECTION("ToAsm") {
        std::vector<uint8_t> bytes = {OP_1, OP_2, OP_ADD};
        CRxdScript script(bytes);
        REQUIRE(script.ToAsm() == "OP_1 OP_2 OP_ADD");
    }
}
