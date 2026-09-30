// ============================================================================
// File: config.cpp
// Description: Parser of the mesh configuration files (DEC-040).
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/app/config.hpp>

namespace vmm {

namespace {

constexpr std::string_view kSpaces = " \t\r";

std::string_view trim(std::string_view s) {
    const auto first = s.find_first_not_of(kSpaces);
    if (first == std::string_view::npos) return {};
    return s.substr(first, s.find_last_not_of(kSpaces) - first + 1);
}

std::unexpected<Error> parse_error(std::size_t line, std::string what) {
    return fail(ErrorCode::ParseError, std::format("line {}: {}", line, what));
}

std::optional<Real> positive_number(std::string_view text) {
    Real value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || !(value > 0)) return std::nullopt;
    return value;
}

bool is_kind(std::string_view kind) { return kind == "region" || kind == "hole" || kind == "background"; }

}  // namespace

std::optional<std::string_view> ConfigSection::find(std::string_view key) const {
    const auto it = std::ranges::find(entries, key, &ConfigEntry::key);
    if (it == entries.end()) return std::nullopt;
    return std::string_view(it->value);
}

Result<MeshConfig> MeshConfig::parse(std::string_view text, std::filesystem::path base_directory) {
    MeshConfig config;
    config.base_directory_ = std::move(base_directory);
    ConfigSection* current = &config.global_;
    std::size_t number = 0;
    while (!text.empty()) {
        const auto eol = text.find('\n');
        std::string_view line = text.substr(0, eol);
        text = eol == std::string_view::npos ? std::string_view{} : text.substr(eol + 1);
        ++number;
        line = trim(line.substr(0, line.find('#')));
        if (line.empty()) continue;
        if (line.front() == '[') {
            if (line.back() != ']') return parse_error(number, "a section header ends with ']'");
            const auto inner = trim(line.substr(1, line.size() - 2));
            const auto space = inner.find_first_of(kSpaces);
            const auto kind = inner.substr(0, space);
            const auto name = space == std::string_view::npos ? std::string_view{} : trim(inner.substr(space));
            if (!is_kind(kind)) {
                return parse_error(number, std::format("unknown section '{}' (region, hole or background)", kind));
            }
            if (kind != "hole" && name.empty()) return parse_error(number, std::format("[{}] needs a name", kind));
            config.sections_.push_back({std::string(kind), std::string(name), number, {}});
            current = &config.sections_.back();
            continue;
        }
        const auto equal = line.find('=');
        if (equal == std::string_view::npos) return parse_error(number, "expected 'key = value' or '[section]'");
        const auto key = trim(line.substr(0, equal));
        const auto value = trim(line.substr(equal + 1));
        if (key.empty() || value.empty()) return parse_error(number, "a key and a value are needed around '='");
        if (current->find(key)) return parse_error(number, std::format("'{}' is repeated", key));
        current->entries.push_back({std::string(key), std::string(value), number});
    }

    for (const auto& entry : config.global_.entries) {
        const auto& [key, value, line] = entry;
        if (key == "dimension") {
            if (value != "2" && value != "3") return parse_error(line, "dimension is 2 or 3");
            config.dimension_ = value == "2" ? 2 : 3;
        } else if (key == "seed") {
            const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), config.seed_);
            if (ec != std::errc{} || end != value.data() + value.size()) {
                return parse_error(line, "seed is a non-negative integer");
            }
        } else if (key == "output") {
            config.output_ = value;
        } else if (key == "formats") {
            std::istringstream words(value);
            config.formats_.assign(std::istream_iterator<std::string>(words), std::istream_iterator<std::string>());
        } else if (key == "interface_pairs" || key == "tolerance") {
            const auto number_value = positive_number(value);
            if (!number_value) return parse_error(line, std::format("{} is a positive number", key));
            (key == "tolerance" ? config.tolerance_ : config.interface_pairs_) = *number_value;
        } else {
            return parse_error(line, std::format("unknown key '{}' (dimension, seed, output, formats, "
                                                 "interface_pairs, tolerance)", key));
        }
    }
    if (config.dimension_ == 0) return fail(ErrorCode::ParseError, "'dimension = 2' or 'dimension = 3' is required");
    if (std::ranges::none_of(config.sections_, [](const ConfigSection& s) { return s.kind == "region"; })) {
        return fail(ErrorCode::ParseError, "no [region] section");
    }
    return config;
}

Result<MeshConfig> MeshConfig::read(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return fail(ErrorCode::FileOpenFailed, file.string());
    const std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    auto config = parse(text, file.parent_path());
    if (!config) return std::unexpected(Error(config.error().code(), std::format("{}: {}", file.string(),
                                                                                 config.error().context())));
    if (!config->global_.find("output")) config->output_ = file.stem();
    return config;
}

std::filesystem::path MeshConfig::output() const { return base_directory_ / output_; }

}  // namespace vmm
