// ============================================================================
// File: exception_log.cpp
// Description: raise() and the logging sink.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <mutex>
#include <source_location>
#include <string>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/error/exception.hpp>
#include <vmm/error/log.hpp>

namespace vmm {
namespace {

std::mutex& sink_mutex() {
    static std::mutex m;
    return m;
}

LogSink& sink() {
    static LogSink s;
    return s;
}

}  // namespace

void raise(ErrorCode code, std::string context, EntityRef entity, std::source_location where) {
    throw Exception(Error(code, std::move(context), entity, Severity::Fatal, where));
}

void set_log_sink(LogSink new_sink) {
    const std::lock_guard lock(sink_mutex());
    sink() = std::move(new_sink);
}

void log(const Error& diagnostic) {
    const std::lock_guard lock(sink_mutex());
    if (sink()) sink()(diagnostic);
}

}  // namespace vmm
