// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_ELECTRUM_H
#define RXD_ELECTRUM_H

#include "rxd_params.h"
#include "rxd_tx.h"
#include "rxd_script.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

namespace rxd {

/**
 * UTXO data from Electrum
 */
struct ElectrumUTXO {
    std::string txid;
    uint32_t vout;
    int64_t value;          // in photons (satoshis)
    std::string scriptPubKey;  // hex
    uint32_t height;        // 0 if unconfirmed
};

/**
 * Transaction reference
 */
struct ElectrumTxRef {
    std::string txid;
    uint32_t height;
};

/**
 * Electrum connection configuration
 */
struct ElectrumConfig {
    std::string host;
    uint16_t port = 50002;
    bool ssl = true;
    uint32_t timeoutMs = 30000;
    Network network = Network::MAINNET;
};

/**
 * Electrum client for fetching transaction and UTXO data
 * 
 * Used by rxdeb to fetch live data for debugging scripts
 * against real on-chain state.
 */
class ElectrumClient {
public:
    /**
     * Construct client with configuration
     */
    explicit ElectrumClient(const ElectrumConfig& config);
    
    /**
     * Construct client for default server on given network
     */
    explicit ElectrumClient(Network network = Network::MAINNET);
    
    ~ElectrumClient();
    
    // Connection management
    
    /**
     * Connect to Electrum server
     * @return true if connected
     */
    bool Connect();
    
    /**
     * Check if connected
     */
    bool IsConnected() const;
    
    /**
     * Disconnect from server
     */
    void Disconnect();
    
    /**
     * Get server version info
     */
    std::string GetServerVersion() const;
    
    // Transaction fetching
    
    /**
     * Get raw transaction by txid
     * @param txid Transaction ID (hex)
     * @return Transaction hex, or nullopt if not found
     */
    std::optional<std::string> GetRawTransaction(const std::string& txid);
    
    /**
     * Get parsed transaction
     */
    std::optional<CRxdTx> GetTransaction(const std::string& txid);
    
    /**
     * Get transaction with full input context (for debugging)
     * Fetches the transaction and all input transactions
     */
    struct TxWithInputs {
        CRxdTx tx;
        std::vector<ElectrumUTXO> inputCoins;
    };
    std::optional<TxWithInputs> GetTransactionWithInputs(const std::string& txid);
    
    // UTXO fetching
    
    /**
     * Get UTXOs for a script hash
     * @param scriptHash SHA256 of scriptPubKey (electrum format)
     */
    std::vector<ElectrumUTXO> GetUTXOs(const std::string& scriptHash);
    
    /**
     * Get UTXOs for a script
     */
    std::vector<ElectrumUTXO> GetUTXOsForScript(const CRxdScript& script);
    
    /**
     * Get UTXOs for an address
     */
    std::vector<ElectrumUTXO> GetUTXOsForAddress(const std::string& address);
    
    // History
    
    /**
     * Get transaction history for a script hash
     */
    std::vector<ElectrumTxRef> GetHistory(const std::string& scriptHash);
    
    /**
     * Get transaction history for an address
     */
    std::vector<ElectrumTxRef> GetHistoryForAddress(const std::string& address);
    
    // Blockchain info
    
    /**
     * Get current block height
     */
    uint32_t GetBlockHeight();
    
    /**
     * Get block header at height
     */
    std::string GetBlockHeader(uint32_t height);
    
    // Broadcasting (for testing)
    
    /**
     * Broadcast raw transaction
     * @return txid if successful
     */
    std::optional<std::string> BroadcastTransaction(const std::string& rawTx);
    
    // Error handling
    
    /**
     * Get last error message
     */
    std::string GetLastError() const;
    
    /**
     * Check if last operation had an error
     */
    bool HasError() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * Parse Electrum server string (host:port)
 */
ElectrumConfig ParseElectrumServer(const std::string& server, Network network = Network::MAINNET);

/**
 * Get default Electrum server for network
 */
ElectrumConfig GetDefaultElectrumServer(Network network);

/**
 * Calculate Electrum script hash from script
 * (SHA256 of scriptPubKey, reversed for Electrum protocol)
 */
std::string CalculateScriptHash(const CRxdScript& script);

} // namespace rxd

#endif // RXD_ELECTRUM_H
