// ============================================================================
// File: log.hpp
// Description: Logging facade: a process-wide callback sink (no virtual
//              interface, R3). Without a sink, messages are dropped.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

#pragma once

//==============================================================================
//  C++ standard library
//==============================================================================
#include <functional>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/error.hpp>

namespace vmm {

using LogSink = std::function<void(const Error&)>;

/// Installs the sink (an empty function removes it). Thread-safe.
void set_log_sink(LogSink sink);

/// Sends a diagnostic (usually a warning) to the sink. Thread-safe.
void log(const Error& diagnostic);

}  // namespace vmm
