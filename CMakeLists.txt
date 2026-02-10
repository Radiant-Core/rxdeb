cmake_minimum_required(VERSION 3.16)
project(rxdeb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build test suite" ON)
option(USE_RADIANT_CORE_SUBMODULE "Use Radiant-Core as submodule for consensus accuracy" ON)

# Find required packages
find_package(Threads REQUIRED)

# Radiant-Core submodule (for consensus-accurate execution)
if(USE_RADIANT_CORE_SUBMODULE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/deps/radiant-core/CMakeLists.txt")
        message(STATUS "Using Radiant-Core submodule for consensus accuracy")
        add_subdirectory(deps/radiant-core EXCLUDE_FROM_ALL)
        set(RADIANT_CORE_AVAILABLE ON)
    else()
        message(WARNING "Radiant-Core submodule not found. Run: git submodule update --init")
        set(RADIANT_CORE_AVAILABLE OFF)
    endif()
endif()

# secp256k1 library
if(EXISTS "${CMAKE_SOURCE_DIR}/secp256k1/CMakeLists.txt")
    add_subdirectory(secp256k1)
    set(SECP256K1_LIBRARY secp256k1)
else()
    find_library(SECP256K1_LIBRARY secp256k1)
    if(NOT SECP256K1_LIBRARY)
        message(FATAL_ERROR "secp256k1 library not found")
    endif()
endif()

# Core library sources (shared between Bitcoin and Radiant backends)
set(CORE_SOURCES
    arith_uint256.cpp
    base58.cpp
    bech32.cpp
    hash.cpp
    pubkey.cpp
    uint256.cpp
    value.cpp
    functions.cpp
    instance.cpp
    merklebranch.cpp
    crypto/blake3.cpp
    crypto/k12.cpp
    crypto/sha256.cpp
    crypto/sha512.cpp
    crypto/ripemd160.cpp
    crypto/hmac_sha256.cpp
    crypto/hmac_sha512.cpp
    support/lockedpool.cpp
    support/cleanse.cpp
    util/strencodings.cpp
    script/interpreter.cpp
    script/script.cpp
    script/script_error.cpp
    debugger/interpreter.cpp
    debugger/script.cpp
)

# Radiant adapter layer sources
set(RXD_SOURCES
    rxd/rxd_params.cpp
    rxd/rxd_script.cpp
    rxd/rxd_tx.cpp
    rxd/rxd_vm_adapter.cpp
    rxd/rxd_context.cpp
    rxd/rxd_electrum.cpp
    rxd/rxd_sighash.cpp
)

# Core library
add_library(rxdeb_core STATIC
    ${CORE_SOURCES}
    ${RXD_SOURCES}
)

target_include_directories(rxdeb_core PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/config
    ${CMAKE_SOURCE_DIR}/compat
    ${CMAKE_SOURCE_DIR}/secp256k1/include
)

target_compile_definitions(rxdeb_core PUBLIC
    HAVE_CONFIG_H
    RXD_SUPPORT
)

target_link_libraries(rxdeb_core PUBLIC
    ${SECP256K1_LIBRARY}
    Threads::Threads
)

if(RADIANT_CORE_AVAILABLE)
    target_compile_definitions(rxdeb_core PUBLIC USE_RADIANT_CORE)
    target_link_libraries(rxdeb_core PUBLIC radiant_script)
endif()

# Main rxdeb executable
add_executable(rxdeb
    btcdeb.cpp
    kerl/kerl.c
)

target_link_libraries(rxdeb PRIVATE
    rxdeb_core
)

# Installation
install(TARGETS rxdeb DESTINATION bin)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # Download Catch2 if not present
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    
    # Test executable
    add_executable(rxdeb_tests
        test/test-btcdeb.cpp
        test/signing.cpp
        test/utils.cpp
        test/value.cpp
        tests/rxd/rxd_script_tests.cpp
        tests/rxd/rxd_vm_tests.cpp
        tests/rxd/rxd_introspection_tests.cpp
        tests/rxd/rxd_reference_tests.cpp
    )
    
    target_link_libraries(rxdeb_tests PRIVATE
        rxdeb_core
        Catch2::Catch2WithMain
    )
    
    target_include_directories(rxdeb_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/test
    )
    
    include(CTest)
    include(Catch)
    catch_discover_tests(rxdeb_tests)
    
    # Convenience target for Radiant-only tests
    add_custom_target(check-rxd
        COMMAND ${CMAKE_CTEST_COMMAND} -R "Rxd" --output-on-failure
        DEPENDS rxdeb_tests
        COMMENT "Running Radiant-specific tests"
    )
endif()

# Generate config header
configure_file(
    ${CMAKE_SOURCE_DIR}/config/bitcoin-config.h.in
    ${CMAKE_BINARY_DIR}/config/bitcoin-config.h
)

# Print configuration summary
message(STATUS "")
message(STATUS "rxdeb Configuration:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:         ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests:          ${BUILD_TESTS}")
message(STATUS "  Radiant-Core:         ${RADIANT_CORE_AVAILABLE}")
message(STATUS "  secp256k1:            ${SECP256K1_LIBRARY}")
message(STATUS "")
