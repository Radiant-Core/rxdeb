// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../test/catch.hpp"
#include "../../rxd/rxd_vm_adapter.h"
#include "../../rxd/rxd_script.h"
#include "../../rxd/rxd_tx.h"
#include "../../rxd/rxd_context.h"

using namespace rxd;

// Helper to build a simple script
CRxdScript BuildScript(std::initializer_list<uint8_t> bytes) {
    return CRxdScript(std::vector<uint8_t>(bytes));
}

// Helper to run a script and check result
bool RunScript(const CRxdScript& script, bool expectSuccess = true) {
    CRxdScript empty;
    CRxdTx dummyTx;
    auto ctx = CreateMinimalContext();
    
    RxdVMAdapter vm(empty, script, dummyTx, 0, 0, ctx);
    bool result = vm.Run();
    
    if (expectSuccess != result) {
        INFO("Error: " << vm.GetErrorString());
    }
    
    return result == expectSuccess;
}

TEST_CASE("Basic arithmetic operations", "[rxd][vm]") {
    SECTION("OP_ADD") {
        // 1 + 2 = 3, 3 == 3 -> true
        CRxdScript script = BuildScript({OP_1, OP_2, OP_ADD, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_SUB") {
        // 5 - 3 = 2
        CRxdScript script = BuildScript({OP_5, OP_3, OP_SUB, OP_2, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_MUL (re-enabled in Radiant)") {
        // 3 * 4 = 12
        CRxdScript script = BuildScript({OP_3, OP_4, OP_MUL, OP_12, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_DIV (re-enabled in Radiant)") {
        // 12 / 3 = 4
        CRxdScript script = BuildScript({OP_12, OP_3, OP_DIV, OP_4, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_MOD") {
        // 13 % 5 = 3
        CRxdScript script = BuildScript({OP_13, OP_5, OP_MOD, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_1ADD") {
        // 5 + 1 = 6
        CRxdScript script = BuildScript({OP_5, OP_1ADD, OP_6, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_1SUB") {
        // 5 - 1 = 4
        CRxdScript script = BuildScript({OP_5, OP_1SUB, OP_4, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_NEGATE") {
        // negate(5) = -5, then add 5 = 0
        CRxdScript script = BuildScript({OP_5, OP_NEGATE, OP_5, OP_ADD, OP_0, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_ABS") {
        // abs(-5) = 5
        CRxdScript script = BuildScript({OP_5, OP_NEGATE, OP_ABS, OP_5, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_2MUL") {
        // 5 * 2 = 10
        CRxdScript script = BuildScript({OP_5, OP_2MUL, OP_10, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_2DIV") {
        // 10 / 2 = 5
        CRxdScript script = BuildScript({OP_10, OP_2DIV, OP_5, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_2DIV truncation") {
        // 7 / 2 = 3 (truncates toward zero)
        CRxdScript script = BuildScript({OP_7, OP_2DIV, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_2MUL then OP_2DIV round-trip") {
        // 3 * 2 / 2 = 3
        CRxdScript script = BuildScript({OP_3, OP_2MUL, OP_2DIV, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_2MUL stack underflow") {
        CRxdScript script = BuildScript({OP_2MUL});
        REQUIRE(RunScript(script, false));
    }
    
    SECTION("OP_2DIV stack underflow") {
        CRxdScript script = BuildScript({OP_2DIV});
        REQUIRE(RunScript(script, false));
    }
}

TEST_CASE("Stack operations", "[rxd][vm]") {
    SECTION("OP_DUP") {
        // 1, dup -> 1, 1, add = 2
        CRxdScript script = BuildScript({OP_1, OP_DUP, OP_ADD, OP_2, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_DROP") {
        // 1, 2, drop -> 1
        CRxdScript script = BuildScript({OP_1, OP_2, OP_DROP});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_SWAP") {
        // 1, 2, swap -> 2, 1, sub = 1
        CRxdScript script = BuildScript({OP_1, OP_2, OP_SWAP, OP_SUB, OP_1, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_ROT") {
        // 1, 2, 3, rot -> 2, 3, 1
        CRxdScript script = BuildScript({OP_1, OP_2, OP_3, OP_ROT, OP_1, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_OVER") {
        // 1, 2, over -> 1, 2, 1, add = 3
        CRxdScript script = BuildScript({OP_1, OP_2, OP_OVER, OP_ADD, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_NIP") {
        // 1, 2, nip -> 2
        CRxdScript script = BuildScript({OP_1, OP_2, OP_NIP, OP_2, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_TUCK") {
        // 1, 2, tuck -> 2, 1, 2
        CRxdScript script = BuildScript({OP_1, OP_2, OP_TUCK, OP_DROP, OP_1, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_2DUP") {
        // 1, 2, 2dup -> 1, 2, 1, 2
        CRxdScript script = BuildScript({OP_1, OP_2, OP_2DUP, OP_ADD, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_DEPTH") {
        // 1, 2, 3, depth -> 1, 2, 3, 3
        CRxdScript script = BuildScript({OP_1, OP_2, OP_3, OP_DEPTH, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_PICK") {
        // 1, 2, 3, 2, pick -> 1, 2, 3, 1
        CRxdScript script = BuildScript({OP_1, OP_2, OP_3, OP_2, OP_PICK, OP_1, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_TOALTSTACK and OP_FROMALTSTACK") {
        // 1, 2, toalt, 3, add, fromalt, add = 6
        CRxdScript script = BuildScript({OP_1, OP_2, OP_TOALTSTACK, OP_3, OP_ADD, 
                                         OP_FROMALTSTACK, OP_ADD, OP_6, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
}

TEST_CASE("Comparison operations", "[rxd][vm]") {
    SECTION("OP_EQUAL") {
        CRxdScript script = BuildScript({OP_5, OP_5, OP_EQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_NUMEQUAL") {
        CRxdScript script = BuildScript({OP_5, OP_5, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_LESSTHAN") {
        CRxdScript script = BuildScript({OP_3, OP_5, OP_LESSTHAN});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_GREATERTHAN") {
        CRxdScript script = BuildScript({OP_5, OP_3, OP_GREATERTHAN});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_LESSTHANOREQUAL") {
        CRxdScript script1 = BuildScript({OP_3, OP_5, OP_LESSTHANOREQUAL});
        REQUIRE(RunScript(script1));
        
        CRxdScript script2 = BuildScript({OP_5, OP_5, OP_LESSTHANOREQUAL});
        REQUIRE(RunScript(script2));
    }
    
    SECTION("OP_MIN") {
        CRxdScript script = BuildScript({OP_3, OP_5, OP_MIN, OP_3, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_MAX") {
        CRxdScript script = BuildScript({OP_3, OP_5, OP_MAX, OP_5, OP_NUMEQUAL});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_WITHIN") {
        // 3 is within [2, 5)
        CRxdScript script = BuildScript({OP_3, OP_2, OP_5, OP_WITHIN});
        REQUIRE(RunScript(script));
    }
}

TEST_CASE("Control flow", "[rxd][vm]") {
    SECTION("OP_IF true branch") {
        // if true then 1 else 0
        CRxdScript script = BuildScript({OP_1, OP_IF, OP_1, OP_ELSE, OP_0, OP_ENDIF});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_IF false branch") {
        // if false then 0 else 1
        CRxdScript script = BuildScript({OP_0, OP_IF, OP_0, OP_ELSE, OP_1, OP_ENDIF});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_NOTIF") {
        // notif false then 1
        CRxdScript script = BuildScript({OP_0, OP_NOTIF, OP_1, OP_ENDIF});
        REQUIRE(RunScript(script));
    }
    
    SECTION("Nested IF") {
        // if true then (if true then 1) else 0
        CRxdScript script = BuildScript({OP_1, OP_IF, OP_1, OP_IF, OP_1, OP_ENDIF, 
                                         OP_ELSE, OP_0, OP_ENDIF});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_VERIFY success") {
        CRxdScript script = BuildScript({OP_1, OP_VERIFY, OP_1});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_VERIFY failure") {
        CRxdScript script = BuildScript({OP_0, OP_VERIFY, OP_1});
        REQUIRE(RunScript(script, false));  // Expect failure
    }
}

TEST_CASE("Boolean operations", "[rxd][vm]") {
    SECTION("OP_BOOLAND") {
        CRxdScript script1 = BuildScript({OP_1, OP_1, OP_BOOLAND});
        REQUIRE(RunScript(script1));
        
        CRxdScript script2 = BuildScript({OP_1, OP_0, OP_BOOLAND, OP_NOT});
        REQUIRE(RunScript(script2));
    }
    
    SECTION("OP_BOOLOR") {
        CRxdScript script = BuildScript({OP_1, OP_0, OP_BOOLOR});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_NOT") {
        CRxdScript script = BuildScript({OP_0, OP_NOT});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_0NOTEQUAL") {
        CRxdScript script = BuildScript({OP_5, OP_0NOTEQUAL});
        REQUIRE(RunScript(script));
    }
}

TEST_CASE("Splice operations (Radiant re-enabled)", "[rxd][vm]") {
    SECTION("OP_CAT") {
        // Push two 1-byte values, concatenate, check size is 2
        std::vector<uint8_t> bytes = {0x01, 0xaa, 0x01, 0xbb, OP_CAT, OP_SIZE, OP_2, OP_NUMEQUAL};
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_SPLIT") {
        // Push 3 bytes, split at position 1, check sizes
        // After split: stack = [[0xaa], [0xbb, 0xcc]]
        // Check right part size (2), drop it, then check left part size (1)
        std::vector<uint8_t> bytes = {0x03, 0xaa, 0xbb, 0xcc, OP_1, OP_SPLIT, 
                                      OP_SIZE, OP_2, OP_NUMEQUAL, OP_VERIFY,
                                      OP_DROP,  // Drop [0xbb, 0xcc]
                                      OP_SIZE, OP_1, OP_NUMEQUAL};
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_SIZE") {
        std::vector<uint8_t> bytes = {0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 
                                      OP_SIZE, OP_5, OP_NUMEQUAL};
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
}

TEST_CASE("Bitwise operations", "[rxd][vm]") {
    SECTION("OP_AND") {
        // 0xff AND 0x0f = 0x0f
        std::vector<uint8_t> bytes = {0x01, 0xff, 0x01, 0x0f, OP_AND, 0x01, 0x0f, OP_EQUAL};
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_OR") {
        // 0xf0 OR 0x0f = 0xff
        std::vector<uint8_t> bytes = {0x01, 0xf0, 0x01, 0x0f, OP_OR, 0x01, 0xff, OP_EQUAL};
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_XOR") {
        // 0xff XOR 0xff = 0x00
        std::vector<uint8_t> bytes = {0x01, 0xff, 0x01, 0xff, OP_XOR, 0x01, 0x00, OP_EQUAL};
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
}

TEST_CASE("VM stepping", "[rxd][vm]") {
    SECTION("Step through script") {
        CRxdScript script = BuildScript({OP_1, OP_2, OP_ADD});
        CRxdScript empty;
        CRxdTx dummyTx;
        auto ctx = CreateMinimalContext();
        
        RxdVMAdapter vm(empty, script, dummyTx, 0, 0, ctx);
        
        REQUIRE(vm.IsDone() == false);
        REQUIRE(vm.IsAtStart() == true);
        
        // Step 1: OP_1
        REQUIRE(vm.Step() == true);
        REQUIRE(vm.GetState().stack.size() == 1);
        
        // Step 2: OP_2
        REQUIRE(vm.Step() == true);
        REQUIRE(vm.GetState().stack.size() == 2);
        
        // Step 3: OP_ADD
        REQUIRE(vm.Step() == true);
        REQUIRE(vm.GetState().stack.size() == 1);
        
        // Script complete
        REQUIRE(vm.Step() == false);
        REQUIRE(vm.IsDone() == true);
    }
    
    SECTION("Rewind") {
        CRxdScript script = BuildScript({OP_1, OP_2, OP_ADD});
        CRxdScript empty;
        CRxdTx dummyTx;
        auto ctx = CreateMinimalContext();
        
        RxdVMAdapter vm(empty, script, dummyTx, 0, 0, ctx);
        
        vm.Step();  // OP_1
        vm.Step();  // OP_2
        REQUIRE(vm.GetState().stack.size() == 2);
        
        vm.Rewind();  // Back to after OP_1
        REQUIRE(vm.GetState().stack.size() == 1);
        
        vm.Rewind();  // Back to start
        REQUIRE(vm.GetState().stack.size() == 0);
        REQUIRE(vm.IsAtStart() == true);
    }
    
    SECTION("Reset") {
        CRxdScript script = BuildScript({OP_1, OP_2, OP_ADD});
        CRxdScript empty;
        CRxdTx dummyTx;
        auto ctx = CreateMinimalContext();
        
        RxdVMAdapter vm(empty, script, dummyTx, 0, 0, ctx);
        
        vm.Run();
        REQUIRE(vm.IsDone() == true);
        
        vm.Reset();
        REQUIRE(vm.IsDone() == false);
        REQUIRE(vm.IsAtStart() == true);
    }
}

TEST_CASE("V2 hard fork hash opcodes", "[rxd][vm][v2]") {
    SECTION("OP_BLAKE3 on empty input") {
        // Push empty bytes, apply OP_BLAKE3, check output is 32 bytes
        // BLAKE3("") = af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262
        std::vector<uint8_t> bytes = {OP_0, OP_BLAKE3, OP_SIZE};
        CRxdScript script(bytes);
        // Push 32 (0x20) and check NUMEQUAL
        script << std::vector<uint8_t>{0x20};
        script << OP_NUMEQUAL;
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_K12 on empty input") {
        // Push empty bytes, apply OP_K12, check output is 32 bytes
        // K12("") = 1ac2d450fc3b4205d19da7bfca1b37513c0803577ac7167f06fe2ce1f0ef39e5
        std::vector<uint8_t> bytes = {OP_0, OP_K12, OP_SIZE};
        CRxdScript script(bytes);
        script << std::vector<uint8_t>{0x20};
        script << OP_NUMEQUAL;
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_BLAKE3 deterministic") {
        // Hash the same data twice, results must be equal
        std::vector<uint8_t> bytes = {
            0x03, 0x61, 0x62, 0x63,  // push "abc"
            OP_DUP,
            OP_BLAKE3,
            OP_SWAP,
            OP_BLAKE3,
            OP_EQUAL
        };
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_K12 deterministic") {
        // Hash the same data twice, results must be equal
        std::vector<uint8_t> bytes = {
            0x03, 0x61, 0x62, 0x63,  // push "abc"
            OP_DUP,
            OP_K12,
            OP_SWAP,
            OP_K12,
            OP_EQUAL
        };
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_BLAKE3 and OP_K12 produce different outputs") {
        // Same input, different hash functions -> different outputs
        std::vector<uint8_t> bytes = {
            0x03, 0x61, 0x62, 0x63,  // push "abc"
            OP_DUP,
            OP_BLAKE3,
            OP_SWAP,
            OP_K12,
            OP_EQUAL,
            OP_NOT  // They should NOT be equal
        };
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_BLAKE3 stack underflow") {
        CRxdScript script = BuildScript({OP_BLAKE3});
        REQUIRE(RunScript(script, false));
    }
    
    SECTION("OP_K12 stack underflow") {
        CRxdScript script = BuildScript({OP_K12});
        REQUIRE(RunScript(script, false));
    }
}

TEST_CASE("V2 hard fork shift opcodes", "[rxd][vm][v2]") {
    SECTION("OP_LSHIFT by 0 bits") {
        // 1 << 0 = 1
        CRxdScript script = BuildScript({OP_1, OP_0, OP_LSHIFT});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_RSHIFT by 0 bits") {
        // 1 >> 0 = 1
        CRxdScript script = BuildScript({OP_1, OP_0, OP_RSHIFT});
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_LSHIFT basic") {
        // Push byte 0x01, shift left by 3 bits -> 0x08
        std::vector<uint8_t> bytes = {
            0x01, 0x01,  // push [0x01]
            OP_3,        // push 3
            OP_LSHIFT,   // [0x01] << 3 = [0x08]
            0x01, 0x08,  // push [0x08]
            OP_EQUAL
        };
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_RSHIFT basic") {
        // Push byte 0x10 (16), shift right by 2 bits -> 0x04
        std::vector<uint8_t> bytes = {
            0x01, 0x10,  // push [0x10]
            OP_2,        // push 2
            OP_RSHIFT,   // [0x10] >> 2 = [0x04]
            0x01, 0x04,  // push [0x04]
            OP_EQUAL
        };
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_LSHIFT cross-byte boundary") {
        // Push [0x01, 0x00], shift left by 8 bits -> [0x00, 0x00] (shifted out)
        // Actually: [0x01] << 8 = [0x00] (only 1 byte)
        // Let's use 2-byte: [0x00, 0x01] << 4 = [0x00, 0x10]
        std::vector<uint8_t> bytes = {
            0x02, 0x00, 0x01,  // push [0x00, 0x01]
            OP_4,              // push 4
            OP_LSHIFT,         // shift left 4 bits
            0x02, 0x00, 0x10,  // push [0x00, 0x10]
            OP_EQUAL
        };
        CRxdScript script(bytes);
        REQUIRE(RunScript(script));
    }
    
    SECTION("OP_LSHIFT stack underflow") {
        CRxdScript script = BuildScript({OP_1, OP_LSHIFT});
        REQUIRE(RunScript(script, false));
    }
    
    SECTION("OP_RSHIFT stack underflow") {
        CRxdScript script = BuildScript({OP_1, OP_RSHIFT});
        REQUIRE(RunScript(script, false));
    }
}

TEST_CASE("Script errors", "[rxd][vm]") {
    SECTION("Stack underflow") {
        CRxdScript script = BuildScript({OP_ADD});  // Needs 2 items
        REQUIRE(RunScript(script, false));
    }
    
    SECTION("OP_RETURN") {
        CRxdScript script = BuildScript({OP_1, OP_RETURN});
        REQUIRE(RunScript(script, false));
    }
    
    SECTION("Unbalanced IF") {
        CRxdScript script = BuildScript({OP_1, OP_IF, OP_1});  // Missing ENDIF
        REQUIRE(RunScript(script, false));
    }
    
    SECTION("Division by zero") {
        CRxdScript script = BuildScript({OP_5, OP_0, OP_DIV});
        REQUIRE(RunScript(script, false));
    }
}
