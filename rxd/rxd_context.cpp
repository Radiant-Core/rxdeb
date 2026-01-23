// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rxd_context.h"
#include "rxd_tx.h"
#include "rxd_script.h"

#include <sstream>
#include <iomanip>

namespace rxd {

struct RxdExecutionContext::Impl {
    std::shared_ptr<const CRxdTx> tx;
    std::vector<Coin> inputCoins;
    unsigned int inputIndex;
    
    CRxdScript activeBytecode;
    
    // Cached reference summaries
    std::vector<PushRefScriptSummary> inputPushRefSummaries;
    std::vector<PushRefScriptSummary> outputPushRefSummaries;
    
    // All references
    std::set<RefType> inputPushRefs;
    std::set<RefType> outputPushRefs;
    
    Impl(std::shared_ptr<const CRxdTx> transaction,
         std::vector<Coin> coins,
         unsigned int idx)
        : tx(transaction)
        , inputCoins(std::move(coins))
        , inputIndex(idx)
    {
        ComputeRefSummaries();
    }
    
    void ComputeRefSummaries() {
        // Compute reference summaries for all inputs
        inputPushRefSummaries.resize(inputCoins.size());
        for (size_t i = 0; i < inputCoins.size(); i++) {
            inputPushRefSummaries[i] = ComputePushRefSummary(inputCoins[i].scriptPubKey);
            for (const auto& ref : inputPushRefSummaries[i].pushRefSet) {
                inputPushRefs.insert(ref);
            }
        }
        
        // Compute reference summaries for all outputs
        if (tx) {
            outputPushRefSummaries.resize(tx->vout.size());
            for (size_t i = 0; i < tx->vout.size(); i++) {
                outputPushRefSummaries[i] = ComputePushRefSummary(tx->vout[i].scriptPubKey);
                for (const auto& ref : outputPushRefSummaries[i].pushRefSet) {
                    outputPushRefs.insert(ref);
                }
            }
        }
    }
    
    PushRefScriptSummary ComputePushRefSummary(const CRxdScript& script) {
        PushRefScriptSummary summary;
        summary.value = 0;
        summary.stateSeparatorByteIndex = 0xFFFFFFFF;
        
        CRxdScript::const_iterator pc = script.begin();
        opcodetype opcode;
        valtype data;
        uint32_t byteIndex = 0;
        
        while (script.GetOp(pc, opcode, data)) {
            switch (opcode) {
                case OP_PUSHINPUTREF:
                    if (data.size() == 36) {
                        summary.pushRefSet.insert(data);
                    }
                    break;
                    
                case OP_REQUIREINPUTREF:
                    if (data.size() == 36) {
                        summary.requireRefSet.insert(data);
                    }
                    break;
                    
                case OP_DISALLOWPUSHINPUTREFSIBLING:
                    if (data.size() == 36) {
                        summary.disallowSiblingRefSet.insert(data);
                    }
                    break;
                    
                case OP_PUSHINPUTREFSINGLETON:
                    if (data.size() == 36) {
                        summary.singletonRefSet.insert(data);
                    }
                    break;
                    
                case OP_STATESEPARATOR:
                    if (summary.stateSeparatorByteIndex == 0xFFFFFFFF) {
                        summary.stateSeparatorByteIndex = byteIndex;
                    }
                    break;
                    
                default:
                    break;
            }
            byteIndex = pc - script.begin();
        }
        
        return summary;
    }
};

RxdExecutionContext::RxdExecutionContext(
    std::shared_ptr<const CRxdTx> tx,
    std::vector<Coin> inputCoins,
    unsigned int inputIndex)
    : impl_(std::make_unique<Impl>(tx, std::move(inputCoins), inputIndex))
{
}

RxdExecutionContext::~RxdExecutionContext() = default;

const CRxdTx& RxdExecutionContext::GetTx() const {
    return *impl_->tx;
}

unsigned int RxdExecutionContext::GetInputIndex() const {
    return impl_->inputIndex;
}

size_t RxdExecutionContext::GetInputCount() const {
    return impl_->tx ? impl_->tx->vin.size() : 0;
}

size_t RxdExecutionContext::GetOutputCount() const {
    return impl_->tx ? impl_->tx->vout.size() : 0;
}

int32_t RxdExecutionContext::GetTxVersion() const {
    return impl_->tx ? impl_->tx->nVersion : 0;
}

uint32_t RxdExecutionContext::GetLockTime() const {
    return impl_->tx ? impl_->tx->nLockTime : 0;
}

const Coin& RxdExecutionContext::GetInputCoin(unsigned int index) const {
    static Coin empty;
    if (index >= impl_->inputCoins.size()) return empty;
    return impl_->inputCoins[index];
}

int64_t RxdExecutionContext::GetUtxoValue(unsigned int index) const {
    if (index >= impl_->inputCoins.size()) return 0;
    return impl_->inputCoins[index].value;
}

const CRxdScript& RxdExecutionContext::GetUtxoBytecode(unsigned int index) const {
    static CRxdScript empty;
    if (index >= impl_->inputCoins.size()) return empty;
    return impl_->inputCoins[index].scriptPubKey;
}

std::vector<uint8_t> RxdExecutionContext::GetOutpointTxHash(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vin.size()) {
        return std::vector<uint8_t>(32, 0);
    }
    return impl_->tx->vin[index].prevout.txid;
}

uint32_t RxdExecutionContext::GetOutpointIndex(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vin.size()) return 0;
    return impl_->tx->vin[index].prevout.n;
}

CRxdScript RxdExecutionContext::GetInputBytecode(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vin.size()) return CRxdScript();
    return impl_->tx->vin[index].scriptSig;
}

uint32_t RxdExecutionContext::GetInputSequence(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vin.size()) return 0;
    return impl_->tx->vin[index].nSequence;
}

int64_t RxdExecutionContext::GetOutputValue(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vout.size()) return 0;
    return impl_->tx->vout[index].nValue;
}

CRxdScript RxdExecutionContext::GetOutputBytecode(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vout.size()) return CRxdScript();
    return impl_->tx->vout[index].scriptPubKey;
}

const CRxdScript& RxdExecutionContext::GetActiveBytecode() const {
    return impl_->activeBytecode;
}

void RxdExecutionContext::SetActiveBytecode(const CRxdScript& script) {
    impl_->activeBytecode = script;
}

uint32_t RxdExecutionContext::GetStateSeparatorIndexUtxo(unsigned int index) const {
    if (index >= impl_->inputPushRefSummaries.size()) return 0xFFFFFFFF;
    return impl_->inputPushRefSummaries[index].stateSeparatorByteIndex;
}

uint32_t RxdExecutionContext::GetStateSeparatorIndexOutput(unsigned int index) const {
    if (index >= impl_->outputPushRefSummaries.size()) return 0xFFFFFFFF;
    return impl_->outputPushRefSummaries[index].stateSeparatorByteIndex;
}

CRxdScript RxdExecutionContext::GetCodeScriptBytecodeUtxo(unsigned int index) const {
    if (index >= impl_->inputCoins.size()) return CRxdScript();
    
    const CRxdScript& script = impl_->inputCoins[index].scriptPubKey;
    uint32_t sepIdx = GetStateSeparatorIndexUtxo(index);
    
    if (sepIdx == 0xFFFFFFFF) {
        // No state separator, entire script is code script
        return script;
    }
    
    // Code script is after the state separator
    return CRxdScript(std::vector<uint8_t>(
        script.begin() + sepIdx + 1,
        script.end()
    ));
}

CRxdScript RxdExecutionContext::GetCodeScriptBytecodeOutput(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vout.size()) return CRxdScript();
    
    const CRxdScript& script = impl_->tx->vout[index].scriptPubKey;
    uint32_t sepIdx = GetStateSeparatorIndexOutput(index);
    
    if (sepIdx == 0xFFFFFFFF) {
        return script;
    }
    
    return CRxdScript(std::vector<uint8_t>(
        script.begin() + sepIdx + 1,
        script.end()
    ));
}

CRxdScript RxdExecutionContext::GetStateScriptBytecodeUtxo(unsigned int index) const {
    if (index >= impl_->inputCoins.size()) return CRxdScript();
    
    const CRxdScript& script = impl_->inputCoins[index].scriptPubKey;
    uint32_t sepIdx = GetStateSeparatorIndexUtxo(index);
    
    if (sepIdx == 0xFFFFFFFF) {
        // No state separator, no state script
        return CRxdScript();
    }
    
    // State script is before the state separator
    return CRxdScript(std::vector<uint8_t>(
        script.begin(),
        script.begin() + sepIdx
    ));
}

CRxdScript RxdExecutionContext::GetStateScriptBytecodeOutput(unsigned int index) const {
    if (!impl_->tx || index >= impl_->tx->vout.size()) return CRxdScript();
    
    const CRxdScript& script = impl_->tx->vout[index].scriptPubKey;
    uint32_t sepIdx = GetStateSeparatorIndexOutput(index);
    
    if (sepIdx == 0xFFFFFFFF) {
        return CRxdScript();
    }
    
    return CRxdScript(std::vector<uint8_t>(
        script.begin(),
        script.begin() + sepIdx
    ));
}

const PushRefScriptSummary& RxdExecutionContext::GetInputPushRefSummary(unsigned int index) const {
    static PushRefScriptSummary empty;
    if (index >= impl_->inputPushRefSummaries.size()) return empty;
    return impl_->inputPushRefSummaries[index];
}

const PushRefScriptSummary& RxdExecutionContext::GetOutputPushRefSummary(unsigned int index) const {
    static PushRefScriptSummary empty;
    if (index >= impl_->outputPushRefSummaries.size()) return empty;
    return impl_->outputPushRefSummaries[index];
}

const std::set<RefType>& RxdExecutionContext::GetInputPushRefs() const {
    return impl_->inputPushRefs;
}

const std::set<RefType>& RxdExecutionContext::GetOutputPushRefs() const {
    return impl_->outputPushRefs;
}

int64_t RxdExecutionContext::GetRefValueSumUtxos(const RefType& ref) const {
    int64_t sum = 0;
    for (size_t i = 0; i < impl_->inputPushRefSummaries.size(); i++) {
        if (impl_->inputPushRefSummaries[i].pushRefSet.count(ref)) {
            sum += impl_->inputCoins[i].value;
        }
    }
    return sum;
}

int64_t RxdExecutionContext::GetRefValueSumOutputs(const RefType& ref) const {
    int64_t sum = 0;
    if (!impl_->tx) return sum;
    
    for (size_t i = 0; i < impl_->outputPushRefSummaries.size(); i++) {
        if (impl_->outputPushRefSummaries[i].pushRefSet.count(ref)) {
            sum += impl_->tx->vout[i].nValue;
        }
    }
    return sum;
}

uint32_t RxdExecutionContext::GetRefOutputCountUtxos(const RefType& ref) const {
    uint32_t count = 0;
    for (const auto& summary : impl_->inputPushRefSummaries) {
        if (summary.pushRefSet.count(ref)) count++;
    }
    return count;
}

uint32_t RxdExecutionContext::GetRefOutputCountOutputs(const RefType& ref) const {
    uint32_t count = 0;
    for (const auto& summary : impl_->outputPushRefSummaries) {
        if (summary.pushRefSet.count(ref)) count++;
    }
    return count;
}

int64_t RxdExecutionContext::GetCodeScriptHashValueSumUtxos(const std::vector<uint8_t>& csh) const {
    // TODO: Implement code script hash matching
    (void)csh;
    return 0;
}

int64_t RxdExecutionContext::GetCodeScriptHashValueSumOutputs(const std::vector<uint8_t>& csh) const {
    // TODO: Implement code script hash matching
    (void)csh;
    return 0;
}

uint32_t RxdExecutionContext::GetCodeScriptHashOutputCountUtxos(const std::vector<uint8_t>& csh) const {
    // TODO: Implement code script hash matching
    (void)csh;
    return 0;
}

uint32_t RxdExecutionContext::GetCodeScriptHashOutputCountOutputs(const std::vector<uint8_t>& csh) const {
    // TODO: Implement code script hash matching
    (void)csh;
    return 0;
}

bool RxdExecutionContext::IsValid() const {
    return impl_->tx != nullptr && impl_->inputIndex < impl_->tx->vin.size();
}

bool RxdExecutionContext::IsValidInputIndex(unsigned int index) const {
    return index < impl_->inputCoins.size();
}

bool RxdExecutionContext::IsValidOutputIndex(unsigned int index) const {
    return impl_->tx && index < impl_->tx->vout.size();
}

std::string RxdExecutionContext::ToString() const {
    std::ostringstream ss;
    
    ss << "=== Execution Context ===\n";
    ss << "Input Index: " << impl_->inputIndex << "\n";
    
    if (impl_->tx) {
        ss << "TX Version: " << impl_->tx->nVersion << "\n";
        ss << "Input Count: " << impl_->tx->vin.size() << "\n";
        ss << "Output Count: " << impl_->tx->vout.size() << "\n";
        ss << "Lock Time: " << impl_->tx->nLockTime << "\n";
    } else {
        ss << "(No transaction)\n";
    }
    
    ss << "\nInput Coins:\n";
    for (size_t i = 0; i < impl_->inputCoins.size(); i++) {
        ss << "  [" << i << "] Value: " << impl_->inputCoins[i].value << " photons\n";
        ss << "      Script: " << impl_->inputCoins[i].scriptPubKey.size() << " bytes\n";
    }
    
    return ss.str();
}

// Factory functions

std::shared_ptr<RxdExecutionContext> CreateMinimalContext() {
    auto tx = std::make_shared<CRxdTx>();
    std::vector<Coin> coins;
    return std::make_shared<RxdExecutionContext>(tx, coins, 0);
}

std::shared_ptr<RxdExecutionContext> CreateContext(
    std::shared_ptr<const CRxdTx> tx,
    const std::vector<Coin>& inputCoins,
    unsigned int inputIndex)
{
    return std::make_shared<RxdExecutionContext>(tx, inputCoins, inputIndex);
}

} // namespace rxd
