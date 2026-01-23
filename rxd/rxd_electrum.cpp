// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_electrum.h"
#include "rxd_tx.h"
#include "rxd_params.h"

#include <sstream>
#include <iomanip>
#include <cstring>
#include <stdexcept>

// For socket communication
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace rxd {

// Simple JSON helpers (minimal implementation)
namespace json {

std::string Escape(const std::string& s) {
    std::ostringstream ss;
    for (char c : s) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << c;
        }
    }
    return ss.str();
}

std::string BuildRequest(const std::string& method, const std::string& params, int id) {
    std::ostringstream ss;
    ss << "{\"jsonrpc\":\"2.0\",\"id\":" << id 
       << ",\"method\":\"" << method << "\",\"params\":" << params << "}\n";
    return ss.str();
}

// Very simple JSON value extraction
std::string ExtractString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos += searchKey.length();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.size()) return "";
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        return json.substr(pos, end - pos);
    }
    
    // Not a string, find end
    size_t end = json.find_first_of(",}]", pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

bool ExtractBool(const std::string& json, const std::string& key) {
    std::string val = ExtractString(json, key);
    return val == "true";
}

} // namespace json

// ElectrumClient implementation

struct ElectrumClient::Impl {
    std::string host;
    uint16_t port;
    bool useSSL;
    int socket;
    bool connected;
    int requestId;
    int timeoutSec;
    std::string lastError;
    std::string serverVersion;
    
    Impl() : port(0), useSSL(false), socket(-1), connected(false), 
             requestId(0), timeoutSec(30) {}
    
    ~Impl() {
        Disconnect();
    }
    
    bool Connect(const std::string& h, uint16_t p, bool ssl) {
        host = h;
        port = p;
        useSSL = ssl;
        
        // Resolve hostname
        struct addrinfo hints{}, *result;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        std::string portStr = std::to_string(port);
        int err = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);
        if (err != 0) {
            lastError = "Failed to resolve hostname: " + host;
            return false;
        }
        
        // Create socket
        socket = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (socket < 0) {
            freeaddrinfo(result);
            lastError = "Failed to create socket";
            return false;
        }
        
        // Set timeout
        struct timeval tv;
        tv.tv_sec = timeoutSec;
        tv.tv_usec = 0;
        setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
        setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
        
        // Connect
        err = ::connect(socket, result->ai_addr, result->ai_addrlen);
        freeaddrinfo(result);
        
        if (err < 0) {
            Close();
            lastError = "Failed to connect to " + host + ":" + std::to_string(port);
            return false;
        }
        
        // TODO: Add SSL/TLS handshake if useSSL is true
        // For now, only plaintext connections are supported
        if (useSSL) {
            Close();
            lastError = "SSL connections not yet implemented";
            return false;
        }
        
        connected = true;
        return true;
    }
    
    void Disconnect() {
        Close();
        connected = false;
    }
    
    void Close() {
        if (socket >= 0) {
#ifdef _WIN32
            closesocket(socket);
#else
            close(socket);
#endif
            socket = -1;
        }
    }
    
    bool Send(const std::string& data) {
        if (!connected) return false;
        
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t n = ::send(socket, data.c_str() + sent, data.size() - sent, 0);
            if (n <= 0) {
                lastError = "Send failed";
                return false;
            }
            sent += n;
        }
        return true;
    }
    
    std::string Receive() {
        if (!connected) return "";
        
        std::string result;
        char buffer[4096];
        
        while (true) {
            ssize_t n = recv(socket, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) break;
            
            buffer[n] = '\0';
            result += buffer;
            
            // Check if we have a complete JSON response (ends with newline)
            if (!result.empty() && result.back() == '\n') break;
        }
        
        return result;
    }
    
    std::string Request(const std::string& method, const std::string& params) {
        if (!connected) {
            lastError = "Not connected";
            return "";
        }
        
        int id = ++requestId;
        std::string request = json::BuildRequest(method, params, id);
        
        if (!Send(request)) return "";
        
        std::string response = Receive();
        
        // Check for error in response
        if (response.find("\"error\"") != std::string::npos &&
            response.find("\"error\":null") == std::string::npos) {
            lastError = json::ExtractString(response, "message");
            if (lastError.empty()) {
                lastError = "Unknown error in response";
            }
            return "";
        }
        
        return response;
    }
};

ElectrumClient::ElectrumClient(const ElectrumConfig& config)
    : impl_(std::make_unique<Impl>())
{
    impl_->host = config.host;
    impl_->port = config.port;
    impl_->useSSL = config.ssl;
    impl_->timeoutSec = config.timeoutMs / 1000;
}

ElectrumClient::ElectrumClient(Network network)
    : impl_(std::make_unique<Impl>())
{
    auto config = GetDefaultElectrumServer(network);
    impl_->host = config.host;
    impl_->port = config.port;
    impl_->useSSL = config.ssl;
    impl_->timeoutSec = config.timeoutMs / 1000;
}

ElectrumClient::~ElectrumClient() = default;

bool ElectrumClient::Connect() {
    return impl_->Connect(impl_->host, impl_->port, impl_->useSSL);
}

void ElectrumClient::Disconnect() {
    impl_->Disconnect();
}

bool ElectrumClient::IsConnected() const {
    return impl_->connected;
}

std::string ElectrumClient::GetLastError() const {
    return impl_->lastError;
}

bool ElectrumClient::HasError() const {
    return !impl_->lastError.empty();
}

std::string ElectrumClient::GetServerVersion() const {
    return impl_->serverVersion;
}

std::optional<CRxdTx> ElectrumClient::GetTransaction(const std::string& txid) {
    std::string params = "[\"" + json::Escape(txid) + "\", true]";
    std::string response = impl_->Request("blockchain.transaction.get", params);
    
    if (response.empty()) return std::nullopt;
    
    // Extract hex from result
    std::string hex = json::ExtractString(response, "result");
    if (hex.empty()) {
        // Try without verbose flag
        params = "[\"" + json::Escape(txid) + "\"]";
        response = impl_->Request("blockchain.transaction.get", params);
        if (response.empty()) return std::nullopt;
        hex = json::ExtractString(response, "result");
        if (hex.empty()) return std::nullopt;
    }
    
    try {
        return CRxdTx::FromHex(hex);
    } catch (const std::exception& e) {
        impl_->lastError = std::string("Failed to parse transaction: ") + e.what();
        return std::nullopt;
    }
}

std::optional<std::string> ElectrumClient::GetRawTransaction(const std::string& txid) {
    std::string params = "[\"" + json::Escape(txid) + "\"]";
    std::string response = impl_->Request("blockchain.transaction.get", params);
    
    if (response.empty()) return std::nullopt;
    
    std::string hex = json::ExtractString(response, "result");
    if (hex.empty()) return std::nullopt;
    
    return hex;
}

std::vector<ElectrumUTXO> ElectrumClient::GetUTXOs(const std::string& scriptHash) {
    std::vector<ElectrumUTXO> utxos;
    
    std::string params = "[\"" + scriptHash + "\"]";
    std::string response = impl_->Request("blockchain.scripthash.listunspent", params);
    
    if (response.empty()) return utxos;
    
    // Parse UTXO list from response
    size_t pos = 0;
    while ((pos = response.find("\"tx_hash\"", pos)) != std::string::npos) {
        ElectrumUTXO utxo;
        
        size_t start = response.find(':', pos) + 1;
        while (start < response.size() && (response[start] == ' ' || response[start] == '"')) start++;
        size_t end = response.find('"', start);
        if (end == std::string::npos) break;
        utxo.txid = response.substr(start, end - start);
        
        pos = response.find("\"tx_pos\"", end);
        if (pos == std::string::npos) break;
        start = response.find(':', pos) + 1;
        utxo.vout = std::stoul(response.substr(start, response.find_first_of(",}", start) - start));
        
        pos = response.find("\"value\"", pos);
        if (pos == std::string::npos) break;
        start = response.find(':', pos) + 1;
        utxo.value = std::stoll(response.substr(start, response.find_first_of(",}", start) - start));
        
        pos = response.find("\"height\"", pos);
        if (pos == std::string::npos) break;
        start = response.find(':', pos) + 1;
        utxo.height = std::stoi(response.substr(start, response.find_first_of(",}", start) - start));
        
        utxos.push_back(utxo);
        pos++;
    }
    
    return utxos;
}

std::vector<ElectrumUTXO> ElectrumClient::GetUTXOsForScript(const CRxdScript& script) {
    std::string scriptHash = CalculateScriptHash(script);
    return GetUTXOs(scriptHash);
}

std::vector<ElectrumUTXO> ElectrumClient::GetUTXOsForAddress(const std::string& address) {
    // TODO: Convert address to script, then get UTXOs
    impl_->lastError = "Address to script conversion not yet implemented";
    return {};
}

std::vector<ElectrumTxRef> ElectrumClient::GetHistory(const std::string& scriptHash) {
    std::vector<ElectrumTxRef> history;
    
    std::string params = "[\"" + scriptHash + "\"]";
    std::string response = impl_->Request("blockchain.scripthash.get_history", params);
    
    if (response.empty()) return history;
    
    size_t pos = 0;
    while ((pos = response.find("\"tx_hash\"", pos)) != std::string::npos) {
        ElectrumTxRef ref;
        
        size_t start = response.find(':', pos) + 1;
        while (start < response.size() && (response[start] == ' ' || response[start] == '"')) start++;
        size_t end = response.find('"', start);
        if (end == std::string::npos) break;
        ref.txid = response.substr(start, end - start);
        
        pos = response.find("\"height\"", pos);
        if (pos == std::string::npos) break;
        start = response.find(':', pos) + 1;
        ref.height = std::stoi(response.substr(start, response.find_first_of(",}", start) - start));
        
        history.push_back(ref);
        pos++;
    }
    
    return history;
}

std::vector<ElectrumTxRef> ElectrumClient::GetHistoryForAddress(const std::string& address) {
    // TODO: Convert address to script hash
    impl_->lastError = "Address to script conversion not yet implemented";
    return {};
}

std::optional<ElectrumClient::TxWithInputs> ElectrumClient::GetTransactionWithInputs(const std::string& txid) {
    auto tx = GetTransaction(txid);
    if (!tx) return std::nullopt;
    
    TxWithInputs result;
    result.tx = *tx;
    
    for (const auto& input : tx->vin) {
        ElectrumUTXO coin;
        coin.txid = "";
        for (uint8_t b : input.prevout.txid) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", b);
            coin.txid += hex;
        }
        coin.vout = input.prevout.n;
        // Fetch the previous transaction to get the value and script
        auto prevTx = GetTransaction(coin.txid);
        if (prevTx && coin.vout < prevTx->vout.size()) {
            coin.value = prevTx->vout[coin.vout].nValue;
            coin.scriptPubKey = prevTx->vout[coin.vout].scriptPubKey.ToHex();
        }
        result.inputCoins.push_back(coin);
    }
    
    return result;
}

uint32_t ElectrumClient::GetBlockHeight() {
    std::string response = impl_->Request("blockchain.headers.subscribe", "[]");
    
    if (response.empty()) return 0;
    
    std::string heightStr = json::ExtractString(response, "height");
    if (heightStr.empty()) return 0;
    
    return static_cast<uint32_t>(std::stoi(heightStr));
}

std::string ElectrumClient::GetBlockHeader(uint32_t height) {
    std::string params = "[" + std::to_string(height) + "]";
    std::string response = impl_->Request("blockchain.block.header", params);
    
    if (response.empty()) return "";
    
    return json::ExtractString(response, "result");
}

std::optional<std::string> ElectrumClient::BroadcastTransaction(const std::string& txHex) {
    std::string params = "[\"" + json::Escape(txHex) + "\"]";
    std::string response = impl_->Request("blockchain.transaction.broadcast", params);
    
    if (response.empty()) return std::nullopt;
    
    std::string txid = json::ExtractString(response, "result");
    if (txid.empty()) return std::nullopt;
    
    return txid;
}

// Helper functions

ElectrumConfig ParseElectrumServer(const std::string& server, Network network) {
    ElectrumConfig config;
    config.network = network;
    
    size_t colonPos = server.rfind(':');
    if (colonPos != std::string::npos) {
        config.host = server.substr(0, colonPos);
        config.port = static_cast<uint16_t>(std::stoi(server.substr(colonPos + 1)));
    } else {
        config.host = server;
        config.port = 50002;
    }
    
    return config;
}

ElectrumConfig GetDefaultElectrumServer(Network network) {
    ElectrumConfig config;
    config.network = network;
    
    switch (network) {
        case Network::MAINNET:
            config.host = "electrum.radiant.ovh";
            config.port = 50002;
            config.ssl = true;
            break;
        case Network::TESTNET:
            config.host = "testnet.radiant.ovh";
            config.port = 60002;
            config.ssl = true;
            break;
        case Network::REGTEST:
            config.host = "localhost";
            config.port = 50001;
            config.ssl = false;
            break;
    }
    
    return config;
}

std::string CalculateScriptHash(const CRxdScript& script) {
    // SHA256 of script, then reverse bytes for Electrum protocol
    // TODO: Implement proper SHA256 hashing
    (void)script;
    return "";
}

} // namespace rxd
