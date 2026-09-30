// ============================================================================
// File: native.cpp
// Description: Native format writer and reader.
// SPDX-License-Identifier: BSD-3-Clause
// ============================================================================

//==============================================================================
//  C++ standard library
//==============================================================================
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

//==============================================================================
//  VoronoiMeshMaker
//==============================================================================
#include <vmm/io/native.hpp>

namespace vmm {
namespace {

bool has_whitespace(const std::string& s) { return s.find_first_of(" \t\r\n") != std::string::npos; }

/// Line-oriented tokenizer with line numbers for error messages.
class Reader {
public:
    explicit Reader(std::istream& in) : in_(in) {}

    bool next_line() {
        while (std::getline(in_, line_)) {
            ++number_;
            if (!line_.empty() && line_.back() == '\r') line_.pop_back();
            if (line_.empty() || line_[0] == '#') continue;
            tokens_.clear();
            std::istringstream ss(line_);
            for (std::string t; ss >> t;) tokens_.push_back(t);
            return true;
        }
        return false;
    }

    [[nodiscard]] const std::vector<std::string>& tokens() const noexcept { return tokens_; }

    std::unexpected<Error> error(const std::string& what) const {
        return fail(ErrorCode::ParseError, std::format("line {}: {}", number_, what));
    }

    /// Next line must be "<keyword> <n...>"; returns the numbers.
    Result<std::vector<std::size_t>> header(std::string_view keyword, std::size_t count) {
        if (!next_line() || tokens_.empty() || tokens_[0] != keyword || tokens_.size() != count + 1) {
            return error(std::format("expected '{}' with {} value(s)", keyword, count));
        }
        std::vector<std::size_t> v;
        for (std::size_t k = 1; k < tokens_.size(); ++k) {
            auto n = to_size(tokens_[k]);
            if (!n) return std::unexpected(n.error());
            v.push_back(*n);
        }
        return v;
    }

    Result<std::size_t> to_size(const std::string& t) const {
        std::size_t v = 0;
        const auto [p, ec] = std::from_chars(t.data(), t.data() + t.size(), v);
        if (ec != std::errc{} || p != t.data() + t.size()) return error("bad integer '" + t + "'");
        return v;
    }

    Result<Real> to_real(const std::string& t) const {
        Real v = 0;
        const auto [p, ec] = std::from_chars(t.data(), t.data() + t.size(), v);
        if (ec != std::errc{} || p != t.data() + t.size()) return error("bad number '" + t + "'");
        return v;
    }

    /// Next data line with exactly n tokens.
    Status row(std::size_t n) {
        if (!next_line() || tokens_.size() != n) return error(std::format("expected {} value(s)", n));
        return {};
    }

private:
    std::istream& in_;
    std::string line_;
    std::vector<std::string> tokens_;
    std::size_t number_ = 0;
};

template <class T>
Result<T> id_of(Reader& r, const std::string& t, std::size_t bound) {
    auto v = r.to_size(t);
    if (!v) return std::unexpected(v.error());
    if (*v >= bound) return fail(ErrorCode::InconsistentData, std::format("id {} out of range {}", *v, bound));
    return T::from_index(*v);
}

}  // namespace

template <std::size_t D>
Status write_native(const Mesh<D>& m, std::ostream& out, const NativeWriteOptions& options) {
    for (const auto& n : m.media()) {
        if (n.empty() || has_whitespace(n)) return fail(ErrorCode::InvalidArgument, "medium name '" + n + "' needs to be one word");
    }
    for (const auto& r : m.regions()) {
        if (r.name.empty() || has_whitespace(r.name)) return fail(ErrorCode::InvalidArgument, "region name '" + r.name + "'");
    }
    for (const auto& p : m.patches()) {
        if (p.name.empty() || has_whitespace(p.name)) return fail(ErrorCode::InvalidArgument, "patch name '" + p.name + "'");
    }
    auto real = [](Real x) { return std::format("{:.17g}", x); };
    out << "vmm-mesh " << native_format_version << "\n";
    out << "dimension " << D << "\n";
    if (!options.generator.empty()) out << "# generator " << options.generator << "\n";
    out << "media " << m.media().size() << "\n";
    for (const auto& n : m.media()) out << n << "\n";
    out << "regions " << m.regions().size() << "\n";
    for (const auto& r : m.regions()) out << r.name << " " << r.medium.value << "\n";
    out << "patches " << m.patches().size() << "\n";
    for (const auto& p : m.patches()) out << p.name << " " << p.start << " " << p.count << "\n";
    out << "points " << m.point_count() << "\n";
    for (const auto& p : m.points()) {
        for (std::size_t k = 0; k < D; ++k) out << (k ? " " : "") << real(p[k]);
        out << "\n";
    }
    out << "faces " << m.face_count() << " " << m.internal_face_count() << "\n";
    for (const FaceId f : m.faces()) {
        const auto v = m.face_vertices(f);
        out << v.size();
        for (const VertexId x : v) out << " " << x.value;
        out << " " << m.owner(f).value;
        if (m.is_internal(f)) out << " " << m.neighbour(f).value;
        out << "\n";
    }
    out << "cells " << m.cell_count() << "\n";
    for (const CellId c : m.cells()) {
        out << m.region(c).value << " " << m.cell_input_sites()[c.index()].value;
        for (std::size_t k = 0; k < D; ++k) out << " " << real(m.site(c)[k]);
        out << "\n";
    }
    out << "end\n";
    if (!out) return fail(ErrorCode::FileOpenFailed, "write error");
    return {};
}

template <std::size_t D>
Status write_native(const Mesh<D>& m, const std::filesystem::path& path, const NativeWriteOptions& options) {
    std::ofstream out(path);
    if (!out) return fail(ErrorCode::FileOpenFailed, path.string());
    return write_native(m, out, options);
}

template <std::size_t D>
Result<Mesh<D>> read_native(std::istream& in) {
    Reader r(in);
    if (!r.next_line() || r.tokens().size() != 2 || r.tokens()[0] != "vmm-mesh") return r.error("not a vmm-mesh file");
    if (r.tokens()[1] != std::to_string(native_format_version)) {
        return fail(ErrorCode::UnsupportedVersion, "vmm-mesh " + r.tokens()[1]);
    }
    auto dim = r.header("dimension", 1);
    if (!dim) return std::unexpected(dim.error());
    if ((*dim)[0] != D) return fail(ErrorCode::InconsistentData, std::format("dimension {} expected {}", (*dim)[0], D));

    MeshData<D> d;
    auto n = r.header("media", 1);
    if (!n) return std::unexpected(n.error());
    for (std::size_t k = 0; k < (*n)[0]; ++k) {
        if (auto ok = r.row(1); !ok) return std::unexpected(ok.error());
        d.media.push_back(r.tokens()[0]);
    }
    n = r.header("regions", 1);
    if (!n) return std::unexpected(n.error());
    for (std::size_t k = 0; k < (*n)[0]; ++k) {
        if (auto ok = r.row(2); !ok) return std::unexpected(ok.error());
        auto m = id_of<MediumId>(r, r.tokens()[1], d.media.size());
        if (!m) return std::unexpected(m.error());
        d.regions.push_back({r.tokens()[0], *m});
    }
    n = r.header("patches", 1);
    if (!n) return std::unexpected(n.error());
    for (std::size_t k = 0; k < (*n)[0]; ++k) {
        if (auto ok = r.row(3); !ok) return std::unexpected(ok.error());
        auto start = r.to_size(r.tokens()[1]);
        auto count = r.to_size(r.tokens()[2]);
        if (!start) return std::unexpected(start.error());
        if (!count) return std::unexpected(count.error());
        d.patches.push_back({r.tokens()[0], *start, *count});
    }
    n = r.header("points", 1);
    if (!n) return std::unexpected(n.error());
    d.points.resize((*n)[0]);
    for (auto& p : d.points) {
        if (auto ok = r.row(D); !ok) return std::unexpected(ok.error());
        for (std::size_t k = 0; k < D; ++k) {
            auto x = r.to_real(r.tokens()[k]);
            if (!x) return std::unexpected(x.error());
            p[k] = *x;
        }
    }
    n = r.header("faces", 2);
    if (!n) return std::unexpected(n.error());
    const std::size_t nf = (*n)[0];
    const std::size_t ni = (*n)[1];
    if (ni > nf) return r.error("more internal faces than faces");
    std::vector<std::size_t> owner_raw;
    std::vector<std::size_t> neighbour_raw;
    for (std::size_t f = 0; f < nf; ++f) {
        if (!r.next_line() || r.tokens().empty()) return r.error("face row expected");
        auto nv = r.to_size(r.tokens()[0]);
        if (!nv) return std::unexpected(nv.error());
        const std::size_t expected = 1 + *nv + 1 + (f < ni ? 1 : 0);
        if (r.tokens().size() != expected) return r.error(std::format("face {}: {} value(s) expected", f, expected));
        std::vector<VertexId> verts;
        for (std::size_t k = 0; k < *nv; ++k) {
            auto v = id_of<VertexId>(r, r.tokens()[1 + k], d.points.size());
            if (!v) return std::unexpected(v.error());
            verts.push_back(*v);
        }
        d.face_vertices.push_row(verts);
        auto o = r.to_size(r.tokens()[1 + *nv]);
        if (!o) return std::unexpected(o.error());
        owner_raw.push_back(*o);
        if (f < ni) {
            auto nb = r.to_size(r.tokens()[2 + *nv]);
            if (!nb) return std::unexpected(nb.error());
            neighbour_raw.push_back(*nb);
        }
    }
    n = r.header("cells", 1);
    if (!n) return std::unexpected(n.error());
    for (std::size_t c = 0; c < (*n)[0]; ++c) {
        if (auto ok = r.row(2 + D); !ok) return std::unexpected(ok.error());
        auto reg = id_of<RegionId>(r, r.tokens()[0], d.regions.size());
        if (!reg) return std::unexpected(reg.error());
        auto site = r.to_size(r.tokens()[1]);
        if (!site) return std::unexpected(site.error());
        Vec<D> x{};
        for (std::size_t k = 0; k < D; ++k) {
            auto v = r.to_real(r.tokens()[2 + k]);
            if (!v) return std::unexpected(v.error());
            x[k] = *v;
        }
        d.cell_region.push_back(*reg);
        d.cell_input_site.push_back(SiteId::from_index(*site));
        d.sites.push_back(x);
    }
    for (const std::size_t o : owner_raw) {
        if (o >= d.sites.size()) return fail(ErrorCode::InconsistentData, std::format("owner {} out of range", o));
        d.owner.push_back(CellId::from_index(o));
    }
    for (const std::size_t nb : neighbour_raw) {
        if (nb >= d.sites.size()) return fail(ErrorCode::InconsistentData, std::format("neighbour {} out of range", nb));
        d.neighbour.push_back(CellId::from_index(nb));
    }
    if (!r.next_line() || r.tokens().size() != 1 || r.tokens()[0] != "end") return r.error("'end' expected");
    auto mesh = Mesh<D>::from_data(std::move(d));
    if (!mesh) return fail(ErrorCode::InconsistentData, mesh.error().context());
    return mesh;
}

template <std::size_t D>
Result<Mesh<D>> read_native(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) return fail(ErrorCode::FileOpenFailed, path.string());
    return read_native<D>(in);
}

template Status write_native(const Mesh<2>&, std::ostream&, const NativeWriteOptions&);
template Status write_native(const Mesh<3>&, std::ostream&, const NativeWriteOptions&);
template Status write_native(const Mesh<2>&, const std::filesystem::path&, const NativeWriteOptions&);
template Status write_native(const Mesh<3>&, const std::filesystem::path&, const NativeWriteOptions&);
template Result<Mesh<2>> read_native<2>(std::istream&);
template Result<Mesh<3>> read_native<3>(std::istream&);
template Result<Mesh<2>> read_native<2>(const std::filesystem::path&);
template Result<Mesh<3>> read_native<3>(const std::filesystem::path&);

}  // namespace vmm
