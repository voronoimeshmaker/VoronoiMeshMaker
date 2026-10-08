// ============================================================================
// File: vmm_mesh.cpp
// Description: vmm-mesh: generates the mesh of a configuration file and
//              writes it (DEC-040). The work is done by vmm::run_config; this
//              file only reads the command line and prints the report.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <filesystem>
#include <format>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/config.hpp>
#include <vmm/app/run.hpp>
#include <vmm/core/version.hpp>
#include <vmm/error/catalog.hpp>

namespace {

constexpr std::string_view kUsage = R"(usage: vmm-mesh [--language pt|en] [--output <base>] <configuration file>
       vmm-mesh --help | --version

Generates a Voronoi finite-volume mesh from a configuration file and writes it
next to the file (formats vmesh and vtu by default). --output replaces the key
"output" of the file (path of the files without extension). Example of a file:

    dimension = 2
    seed = 42

    [region block]
    shape = rectangle
    lo = 0 0
    hi = 2 1
    sites = uniform
    sites.spacing = 0.05

See the "Configuration files" page of the manual for every key.)";

int usage_error(std::string_view what) {
    std::println(stderr, "vmm-mesh: {}\n\n{}", what, kUsage);
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    const std::span<char*> args(argv, static_cast<std::size_t>(argc));
    std::filesystem::path file;
    std::optional<std::string> output;
    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        if (arg == "--help" || arg == "-h") {
            std::println("{}", kUsage);
            return 0;
        }
        if (arg == "--version") {
            std::println("vmm-mesh {}", vmm::version_string);
            return 0;
        }
        if (arg == "--language") {
            const std::string_view language = i + 1 < args.size() ? std::string_view(args[++i]) : "";
            if (language != "pt" && language != "en") return usage_error("--language takes pt or en");
            vmm::set_language(language == "pt" ? vmm::Language::Portuguese : vmm::Language::English);
            continue;
        }
        if (arg == "--output" || arg == "-o") {
            if (i + 1 >= args.size()) return usage_error("--output takes a path");
            output = args[++i];
            continue;
        }
        if (arg.starts_with("-") || !file.empty()) return usage_error(std::format("unexpected argument '{}'", arg));
        file = arg;
    }
    if (file.empty()) return usage_error("no configuration file");

    auto config = vmm::MeshConfig::read(file);
    if (!config) {
        std::println(stderr, "vmm-mesh: {}", config.error().message());
        return 1;
    }
    if (output) config->set_output(std::filesystem::absolute(*output));
    std::println("vmm-mesh {}: {} ({}D)", vmm::version_string, file.string(), config->dimension());
    const auto report = vmm::run_config(*config);
    if (!report) {
        std::println(stderr, "vmm-mesh: {}", report.error().message());
        return 1;
    }
    std::println("cells {} | internal faces {} | boundary faces {}", report->cells, report->internal_faces,
                 report->boundary_faces);
    std::println("invariants: relative measure error {:.1e}, closure {:.1e}", report->invariants.total_relative_error,
                 report->invariants.max_closure);
    if (report->cvt) {
        const auto& cvt=*report->cvt;
        std::println("CVT: {} accepted iterations | converged {} | stalled {} | energy {:.8e} -> {:.8e}",
            cvt.relative_displacement.size(),cvt.converged,cvt.stalled,cvt.energy.front(),cvt.energy.back());
    }
    for (const auto& path : report->written) std::println("wrote {}", path.string());
    return 0;
}
