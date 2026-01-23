// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_params.h"
#include <stdexcept>
#include <algorithm>

namespace rxd {

const ChainParams& ChainParams::Mainnet() {
    static const ChainParams params = {
        .name = "mainnet",
        .pubkeyPrefix = 0x00,
        .scriptPrefix = 0x05,
        .privateKeyPrefix = 0x80,
        .magicBytes = 0xd9b4bef9,
        .defaultPort = 7333,
        .defaultElectrumPort = 50002,
        .electrumServers = {
            "electrum.radiant.ovh",
            "electrum.radiantblockchain.org"
        }
    };
    return params;
}

const ChainParams& ChainParams::Testnet() {
    static const ChainParams params = {
        .name = "testnet",
        .pubkeyPrefix = 0x6f,
        .scriptPrefix = 0xc4,
        .privateKeyPrefix = 0xef,
        .magicBytes = 0x0709110b,
        .defaultPort = 17333,
        .defaultElectrumPort = 50002,
        .electrumServers = {
            "testnet-electrum.radiant.ovh"
        }
    };
    return params;
}

const ChainParams& ChainParams::Regtest() {
    static const ChainParams params = {
        .name = "regtest",
        .pubkeyPrefix = 0x6f,
        .scriptPrefix = 0xc4,
        .privateKeyPrefix = 0xef,
        .magicBytes = 0xdab5bffa,
        .defaultPort = 18444,
        .defaultElectrumPort = 50002,
        .electrumServers = {}
    };
    return params;
}

const ChainParams& ChainParams::Get(Network network) {
    switch (network) {
        case Network::MAINNET:
            return Mainnet();
        case Network::TESTNET:
            return Testnet();
        case Network::REGTEST:
            return Regtest();
        default:
            throw std::runtime_error("Unknown network");
    }
}

std::string NetworkName(Network network) {
    switch (network) {
        case Network::MAINNET:
            return "mainnet";
        case Network::TESTNET:
            return "testnet";
        case Network::REGTEST:
            return "regtest";
        default:
            return "unknown";
    }
}

Network ParseNetwork(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower == "mainnet" || lower == "main" || lower == "livenet") {
        return Network::MAINNET;
    } else if (lower == "testnet" || lower == "test") {
        return Network::TESTNET;
    } else if (lower == "regtest" || lower == "reg") {
        return Network::REGTEST;
    }
    
    throw std::runtime_error("Unknown network: " + name);
}

} // namespace rxd
