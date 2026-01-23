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
