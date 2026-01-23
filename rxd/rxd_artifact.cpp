// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_artifact.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace rxd {

// Simple JSON parsing helpers
namespace {

std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.size() || json[pos] != '"') return "";
    pos++;
    
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            pos++;
            switch (json[pos]) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                default: result += json[pos];
            }
        } else {
            result += json[pos];
        }
        pos++;
    }
    return result;
}

std::string ExtractJsonArray(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos = json.find('[', pos);
    if (pos == std::string::npos) return "";
    
    int depth = 1;
    size_t start = pos;
    pos++;
    
    while (pos < json.size() && depth > 0) {
        if (json[pos] == '[') depth++;
        else if (json[pos] == ']') depth--;
        else if (json[pos] == '"') {
            pos++;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\') pos++;
                pos++;
            }
        }
        pos++;
    }
    
    return json.substr(start, pos - start);
}

std::string ExtractJsonObject(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    
    pos = json.find('{', pos);
    if (pos == std::string::npos) return "";
    
    int depth = 1;
    size_t start = pos;
    pos++;
    
    while (pos < json.size() && depth > 0) {
        if (json[pos] == '{') depth++;
        else if (json[pos] == '}') depth--;
        else if (json[pos] == '"') {
            pos++;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\') pos++;
                pos++;
            }
        }
        pos++;
    }
    
    return json.substr(start, pos - start);
}

std::vector<std::string> SplitJsonArray(const std::string& arr) {
    std::vector<std::string> result;
    if (arr.size() < 2 || arr[0] != '[') return result;
    
    size_t pos = 1;
    int depth = 0;
    size_t start = pos;
    
    while (pos < arr.size() - 1) {
        char c = arr[pos];
        if (c == '{' || c == '[') depth++;
        else if (c == '}' || c == ']') depth--;
        else if (c == '"') {
            pos++;
            while (pos < arr.size() && arr[pos] != '"') {
                if (arr[pos] == '\\') pos++;
                pos++;
            }
        } else if (c == ',' && depth == 0) {
            result.push_back(Trim(arr.substr(start, pos - start)));
            start = pos + 1;
        }
        pos++;
    }
    
    std::string last = Trim(arr.substr(start, pos - start));
    if (!last.empty()) result.push_back(last);
    
    return result;
}

std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> result;
    std::string h = hex;
    if (h.substr(0, 2) == "0x") h = h.substr(2);
    
    if (h.size() % 2 != 0) h = "0" + h;
    
    for (size_t i = 0; i < h.size(); i += 2) {
        uint8_t byte = 0;
        for (int j = 0; j < 2; j++) {
            char c = h[i + j];
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

struct Artifact::Impl {
    std::string name;
    std::string bytecodeHex;
    CRxdScript bytecode;
    std::string source;
    std::string compilerVersion;
    
    std::vector<ArtifactParam> constructorParams;
    std::vector<ArtifactFunction> functions;
    std::map<size_t, ArtifactSourceMap> sourceMap;
    std::vector<std::string> sourceLines;
    
    bool ParseJson(const std::string& json) {
        // Extract contract name
        name = ExtractJsonString(json, "contractName");
        if (name.empty()) {
            name = ExtractJsonString(json, "name");
        }
        
        // Extract bytecode
        bytecodeHex = ExtractJsonString(json, "bytecode");
        if (!bytecodeHex.empty()) {
            auto bytes = HexToBytes(bytecodeHex);
            bytecode = CRxdScript(bytes);
        }
        
        // Extract source
        source = ExtractJsonString(json, "source");
        if (!source.empty()) {
            // Split into lines
            std::istringstream ss(source);
            std::string line;
            while (std::getline(ss, line)) {
                sourceLines.push_back(line);
            }
        }
        
        // Extract compiler info
        std::string compilerObj = ExtractJsonObject(json, "compiler");
        if (!compilerObj.empty()) {
            compilerVersion = ExtractJsonString(compilerObj, "version");
        }
        
        // Parse constructor inputs
        std::string ctorInputs = ExtractJsonArray(json, "constructorInputs");
        if (!ctorInputs.empty()) {
            auto items = SplitJsonArray(ctorInputs);
            for (const auto& item : items) {
                ArtifactParam param;
                param.name = ExtractJsonString(item, "name");
                param.type = ExtractJsonString(item, "type");
                if (!param.name.empty()) {
                    constructorParams.push_back(param);
                }
            }
        }
        
        // Parse ABI (functions)
        std::string abi = ExtractJsonArray(json, "abi");
        if (!abi.empty()) {
            auto items = SplitJsonArray(abi);
            for (const auto& item : items) {
                ArtifactFunction func;
                func.name = ExtractJsonString(item, "name");
                func.opcodeIndex = 0;
                
                std::string inputs = ExtractJsonArray(item, "inputs");
                if (!inputs.empty()) {
                    auto params = SplitJsonArray(inputs);
                    for (const auto& p : params) {
                        ArtifactParam param;
                        param.name = ExtractJsonString(p, "name");
                        param.type = ExtractJsonString(p, "type");
                        if (!param.name.empty()) {
                            func.params.push_back(param);
                        }
                    }
                }
                
                if (!func.name.empty()) {
                    functions.push_back(func);
                }
            }
        }
        
        // Parse source map if available
        std::string srcMap = ExtractJsonArray(json, "sourceMap");
        if (!srcMap.empty()) {
            auto items = SplitJsonArray(srcMap);
            for (const auto& item : items) {
                ArtifactSourceMap entry;
                std::string opcStr = ExtractJsonString(item, "opcode");
                if (opcStr.empty()) continue;
                
                entry.opcodeIndex = std::stoul(opcStr);
                
                std::string range = ExtractJsonObject(item, "range");
                if (!range.empty()) {
                    std::string startStr = ExtractJsonString(range, "start");
                    std::string endStr = ExtractJsonString(range, "end");
                    // Parse line:column format
                    // Simplified - real implementation needs proper parsing
                }
                
                entry.statement = ExtractJsonString(item, "statement");
                sourceMap[entry.opcodeIndex] = entry;
            }
        }
        
        return !name.empty();
    }
};

Artifact::Artifact() : impl_(std::make_unique<Impl>()) {}
Artifact::~Artifact() = default;

Artifact::Artifact(Artifact&& other) noexcept = default;
Artifact& Artifact::operator=(Artifact&& other) noexcept = default;

std::optional<Artifact> Artifact::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }
    
    std::ostringstream ss;
    ss << file.rdbuf();
    return LoadFromJson(ss.str());
}

std::optional<Artifact> Artifact::LoadFromJson(const std::string& json) {
    Artifact artifact;
    if (!artifact.impl_->ParseJson(json)) {
        return std::nullopt;
    }
    return artifact;
}

const std::string& Artifact::GetName() const {
    return impl_->name;
}

const CRxdScript& Artifact::GetBytecode() const {
    return impl_->bytecode;
}

const std::string& Artifact::GetBytecodeHex() const {
    return impl_->bytecodeHex;
}

const std::string& Artifact::GetSource() const {
    return impl_->source;
}

const std::vector<ArtifactParam>& Artifact::GetConstructorParams() const {
    return impl_->constructorParams;
}

const std::vector<ArtifactFunction>& Artifact::GetFunctions() const {
    return impl_->functions;
}

std::optional<ArtifactFunction> Artifact::GetFunction(const std::string& name) const {
    for (const auto& func : impl_->functions) {
        if (func.name == name) {
            return func;
        }
    }
    return std::nullopt;
}

bool Artifact::HasSourceMap() const {
    return !impl_->sourceMap.empty();
}

std::optional<ArtifactSourceMap> Artifact::GetSourceLocation(size_t opcodeIndex) const {
    auto it = impl_->sourceMap.find(opcodeIndex);
    if (it != impl_->sourceMap.end()) {
        return it->second;
    }
    
    // Find nearest previous entry
    for (auto rit = impl_->sourceMap.rbegin(); rit != impl_->sourceMap.rend(); ++rit) {
        if (rit->first <= opcodeIndex) {
            return rit->second;
        }
    }
    
    return std::nullopt;
}

std::string Artifact::GetSourceLine(size_t lineNumber) const {
    if (lineNumber == 0 || lineNumber > impl_->sourceLines.size()) {
        return "";
    }
    return impl_->sourceLines[lineNumber - 1];
}

std::string Artifact::GetCompilerVersion() const {
    return impl_->compilerVersion;
}

CRxdScript Artifact::Instantiate(const std::vector<std::vector<uint8_t>>& constructorArgs) const {
    // For RadiantScript, constructor params are pushed before the bytecode
    CRxdScript script;
    
    for (const auto& arg : constructorArgs) {
        script << arg;
    }
    
    // Append the bytecode by creating a combined vector
    std::vector<uint8_t> result = script.data();
    const auto& bc = impl_->bytecode.data();
    result.insert(result.end(), bc.begin(), bc.end());
    
    return CRxdScript(result);
}

CRxdScript Artifact::BuildUnlockingScript(
    const std::string& functionName,
    const std::vector<std::vector<uint8_t>>& args) const
{
    CRxdScript script;
    
    // Push function arguments in reverse order (stack convention)
    for (auto it = args.rbegin(); it != args.rend(); ++it) {
        script << *it;
    }
    
    // For multi-function contracts, we may need to push a function selector
    // This depends on the RadiantScript compilation strategy
    auto func = GetFunction(functionName);
    if (func && impl_->functions.size() > 1) {
        // Find function index
        for (size_t i = 0; i < impl_->functions.size(); i++) {
            if (impl_->functions[i].name == functionName) {
                // Push function index if needed
                // This is contract-specific
                break;
            }
        }
    }
    
    return script;
}

bool Artifact::ValidateArgs(
    const std::vector<ArtifactParam>& params,
    const std::vector<std::vector<uint8_t>>& args,
    std::string& error) const
{
    if (params.size() != args.size()) {
        error = "Expected " + std::to_string(params.size()) + 
                " arguments, got " + std::to_string(args.size());
        return false;
    }
    
    for (size_t i = 0; i < params.size(); i++) {
        const auto& param = params[i];
        const auto& arg = args[i];
        
        // Validate based on type
        if (param.type == "bytes20" && arg.size() != 20) {
            error = "Parameter '" + param.name + "' expected 20 bytes, got " + 
                    std::to_string(arg.size());
            return false;
        }
        if (param.type == "bytes32" && arg.size() != 32) {
            error = "Parameter '" + param.name + "' expected 32 bytes, got " + 
                    std::to_string(arg.size());
            return false;
        }
        if (param.type == "pubkey" && arg.size() != 33 && arg.size() != 65) {
            error = "Parameter '" + param.name + "' expected pubkey (33 or 65 bytes), got " + 
                    std::to_string(arg.size());
            return false;
        }
    }
    
    return true;
}

// Helper functions

std::vector<std::vector<uint8_t>> ParseFunctionArgs(
    const std::string& jsonArgs,
    const std::vector<ArtifactParam>& params)
{
    std::vector<std::vector<uint8_t>> result;
    
    // Parse JSON array
    std::string arr = Trim(jsonArgs);
    if (arr.empty() || arr[0] != '[') {
        return result;
    }
    
    auto items = SplitJsonArray(arr);
    
    for (size_t i = 0; i < items.size() && i < params.size(); i++) {
        std::string item = Trim(items[i]);
        result.push_back(TypedValueToBytes(params[i].type, item));
    }
    
    return result;
}

std::vector<uint8_t> TypedValueToBytes(
    const std::string& type,
    const std::string& value)
{
    std::string v = Trim(value);
    
    // Remove quotes if present
    if (v.size() >= 2 && v[0] == '"' && v.back() == '"') {
        v = v.substr(1, v.size() - 2);
    }
    
    // Hex string
    if (v.substr(0, 2) == "0x") {
        return HexToBytes(v);
    }
    
    // Boolean
    if (type == "bool") {
        if (v == "true" || v == "1") {
            return {0x01};
        }
        return {};  // false = empty
    }
    
    // Integer
    if (type == "int" || type == "int64") {
        int64_t n = std::stoll(v);
        if (n == 0) return {};
        
        std::vector<uint8_t> result;
        bool neg = n < 0;
        uint64_t abs_n = neg ? -n : n;
        
        while (abs_n) {
            result.push_back(abs_n & 0xff);
            abs_n >>= 8;
        }
        
        if (result.back() & 0x80) {
            result.push_back(neg ? 0x80 : 0x00);
        } else if (neg) {
            result.back() |= 0x80;
        }
        
        return result;
    }
    
    // Default: treat as hex or raw bytes
    if (v.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos) {
        return HexToBytes(v);
    }
    
    // String literal
    return std::vector<uint8_t>(v.begin(), v.end());
}

} // namespace rxd
