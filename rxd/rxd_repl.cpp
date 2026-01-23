// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdio>
#include <string>

#include "rxd_repl.h"
#include "rxd_params.h"
#include "rxd_script.h"
#include "../debugger/interpreter.h"

#ifdef RXD_SUPPORT

// External references to debugger state
extern InterpreterEnv* env;

namespace rxd {

// Global state for Radiant-specific debugging
static std::shared_ptr<RxdExecutionContext> g_rxd_context;
static RxdArtifact g_artifact;
static bool g_show_refs = false;

void SetExecutionContext(std::shared_ptr<RxdExecutionContext> ctx) {
    g_rxd_context = ctx;
}

void SetArtifact(const RxdArtifact& artifact) {
    g_artifact = artifact;
}

void SetShowRefs(bool show) {
    g_show_refs = show;
}

} // namespace rxd

// REPL command: refs
// Shows the current reference tracking state
int fn_refs(const char* arg) {
    (void)arg;  // unused
    
    printf("=== Reference Tracking State ===\n");
    
    if (!rxd::g_rxd_context) {
        printf("(no execution context available)\n");
        printf("Use --tx and --txin to provide transaction context\n");
        return 0;
    }
    
    // TODO: Implement actual reference tracking display
    // This requires access to the VM's internal reference tracking state
    printf("Push Refs:      (tracking not yet implemented)\n");
    printf("Require Refs:   (tracking not yet implemented)\n");
    printf("Singleton Refs: (tracking not yet implemented)\n");
    printf("\nNote: Full reference tracking requires VM integration.\n");
    
    return 0;
}

// REPL command: context
// Shows the execution context (transaction info for introspection)
int fn_context(const char* arg) {
    (void)arg;  // unused
    
    printf("=== Execution Context ===\n");
    
    if (!rxd::g_rxd_context) {
        printf("(no execution context available)\n");
        printf("Use --tx and --txin, or --electrum with --txid to provide context\n");
        return 0;
    }
    
    auto& ctx = *rxd::g_rxd_context;
    
    printf("Input Index:    %u\n", ctx.GetInputIndex());
    printf("TX Version:     %d\n", ctx.GetTxVersion());
    printf("Input Count:    %zu\n", ctx.GetInputCount());
    printf("Output Count:   %zu\n", ctx.GetOutputCount());
    printf("Lock Time:      %u\n", ctx.GetLockTime());
    printf("\n");
    
    // Show current input UTXO
    if (ctx.IsValidInputIndex(ctx.GetInputIndex())) {
        const auto& coin = ctx.GetInputCoin(ctx.GetInputIndex());
        printf("Current Input UTXO:\n");
        printf("  Value:        %lld photons\n", (long long)coin.value);
        printf("  Script size:  %zu bytes\n", coin.scriptPubKey.size());
    }
    
    return 0;
}

// REPL command: source
// Shows the RadiantScript source location if available
int fn_source(const char* arg) {
    (void)arg;  // unused
    
    printf("=== RadiantScript Source ===\n");
    
    if (rxd::g_artifact.name.empty()) {
        printf("(no artifact loaded)\n");
        printf("Use --artifact=<file.json> to load a RadiantScript artifact\n");
        return 0;
    }
    
    printf("Contract: %s\n", rxd::g_artifact.name.c_str());
    
    if (!rxd::g_artifact.HasSourceMap()) {
        printf("(no source map available - compile with --debug)\n");
        return 0;
    }
    
    // TODO: Get current PC from env and look up source location
    // This requires integration with the VM stepping
    size_t pc = 0;  // env->pc when available
    
    auto loc = rxd::g_artifact.GetSourceLocation(pc);
    if (loc) {
        printf("\n%s:%u:%u\n", loc->file.c_str(), loc->line, loc->column);
        if (!loc->functionName.empty()) {
            printf("  function: %s\n", loc->functionName.c_str());
        }
        
        // TODO: Show actual source line from artifact
        printf("\n  (source display not yet implemented)\n");
    } else {
        printf("\n(no source mapping for current position)\n");
    }
    
    return 0;
}

#endif // RXD_SUPPORT
