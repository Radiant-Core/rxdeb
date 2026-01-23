// Copyright (c) 2024-2026 The Radiant Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RXD_REPL_H
#define RXD_REPL_H

#ifdef RXD_SUPPORT

#include <memory>
#include "rxd_context.h"
#include "rxd_vm_adapter.h"

namespace rxd {

/**
 * Set the execution context for REPL commands
 */
void SetExecutionContext(std::shared_ptr<RxdExecutionContext> ctx);

/**
 * Set the RadiantScript artifact for source-level debugging
 */
void SetArtifact(const RxdArtifact& artifact);

/**
 * Enable/disable reference tracking display
 */
void SetShowRefs(bool show);

} // namespace rxd

// REPL command functions (called from kerl)
extern "C" {
    int fn_refs(const char* arg);
    int fn_context(const char* arg);
    int fn_source(const char* arg);
}

#endif // RXD_SUPPORT

#endif // RXD_REPL_H
