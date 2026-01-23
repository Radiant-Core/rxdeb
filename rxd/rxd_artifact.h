// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_ARTIFACT_H
#define RXD_ARTIFACT_H

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

#include "rxd_script.h"

namespace rxd {

/**
 * Represents a function parameter in a RadiantScript contract
 */
struct ArtifactParam {
    std::string name;
    std::string type;  // "int", "bool", "bytes", "bytes20", "bytes32", "sig", "pubkey", etc.
};

/**
 * Represents a function in a RadiantScript contract
 */
struct ArtifactFunction {
    std::string name;
    std::vector<ArtifactParam> params;
    size_t opcodeIndex;  // Starting opcode index in the script
};

/**
 * Source map entry for debugging
 */
struct ArtifactSourceMap {
    size_t opcodeIndex;
    size_t startLine;
    size_t startColumn;
    size_t endLine;
    size_t endColumn;
    std::string statement;  // The source statement
};

/**
 * RadiantScript compiled artifact
 * 
 * This represents the JSON output from the RadiantScript compiler (radc).
 * Format:
 * {
 *   "contractName": "MyContract",
 *   "constructorInputs": [...],
 *   "abi": [...],
 *   "bytecode": "...",
 *   "source": "...",
 *   "compiler": {...},
 *   "updatedAt": "..."
 * }
 */
class Artifact {
public:
    Artifact();
    ~Artifact();
    
    // Move constructor and assignment (for std::optional compatibility)
    Artifact(Artifact&& other) noexcept;
    Artifact& operator=(Artifact&& other) noexcept;
    
    // Disable copy
    Artifact(const Artifact&) = delete;
    Artifact& operator=(const Artifact&) = delete;
    
    /**
     * Load artifact from JSON file
     */
    static std::optional<Artifact> LoadFromFile(const std::string& path);
    
    /**
     * Load artifact from JSON string
     */
    static std::optional<Artifact> LoadFromJson(const std::string& json);
    
    /**
     * Get the contract name
     */
    const std::string& GetName() const;
    
    /**
     * Get the compiled bytecode as a script
     */
    const CRxdScript& GetBytecode() const;
    
    /**
     * Get the bytecode as hex string
     */
    const std::string& GetBytecodeHex() const;
    
    /**
     * Get the original source code
     */
    const std::string& GetSource() const;
    
    /**
     * Get constructor parameters
     */
    const std::vector<ArtifactParam>& GetConstructorParams() const;
    
    /**
     * Get all functions (ABI)
     */
    const std::vector<ArtifactFunction>& GetFunctions() const;
    
    /**
     * Find a function by name
     */
    std::optional<ArtifactFunction> GetFunction(const std::string& name) const;
    
    /**
     * Check if source map is available
     */
    bool HasSourceMap() const;
    
    /**
     * Get source location for an opcode index
     */
    std::optional<ArtifactSourceMap> GetSourceLocation(size_t opcodeIndex) const;
    
    /**
     * Get the source line for a given line number
     */
    std::string GetSourceLine(size_t lineNumber) const;
    
    /**
     * Get compiler information
     */
    std::string GetCompilerVersion() const;
    
    /**
     * Instantiate the contract with constructor arguments
     * Returns the script with constructor params filled in
     */
    CRxdScript Instantiate(const std::vector<std::vector<uint8_t>>& constructorArgs) const;
    
    /**
     * Build unlocking script for a function call
     */
    CRxdScript BuildUnlockingScript(
        const std::string& functionName,
        const std::vector<std::vector<uint8_t>>& args) const;
    
    /**
     * Validate that arguments match the expected types
     */
    bool ValidateArgs(
        const std::vector<ArtifactParam>& params,
        const std::vector<std::vector<uint8_t>>& args,
        std::string& error) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Parse a JSON value as function arguments
 * Supports:
 * - Integers: 123, -456
 * - Hex strings: "0x1234..."
 * - Booleans: true, false
 * - Arrays: [1, 2, 3]
 */
std::vector<std::vector<uint8_t>> ParseFunctionArgs(
    const std::string& jsonArgs,
    const std::vector<ArtifactParam>& params);

/**
 * Convert a typed value to script bytes
 */
std::vector<uint8_t> TypedValueToBytes(
    const std::string& type,
    const std::string& value);

} // namespace rxd

#endif // RXD_ARTIFACT_H
