#include <test/catch.hpp>

#include <datasets.h>

// Note: Original btcdeb tests used segwit transaction files which are not
// applicable to Radiant. This test verifies the string_from_file utility
// with an existing file instead.
TEST_CASE("Loading strings from files", "[load-strings]") {
    SECTION("Loading from existing file") {
        // Test that string_from_file works with an existing file
        std::string s = string_from_file("README.md");
        REQUIRE(s.size() > 0);
        REQUIRE(s.find("rxdeb") != std::string::npos);
    }
}
