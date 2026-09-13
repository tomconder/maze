// Asset converter. Reads one source model and writes the baked .spnga the
// engine loads. CMake drives one invocation per entry in assets/manifest.json.
//
// Usage: assetconv <source> <output.spnga>
//        assetconv --verify <source> <output.spnga>
//        assetconv --atlas <output.ktx2> <name>=<png> ...
//        assetconv --texture <output.ktx2> <input.png>

#include "atlas.hpp"
#include "logging/log.hpp"
#include "scene/assetformat.hpp"
#include "scene/gltfimport.hpp"
#include "scene/ktx2.hpp"
#include "scene/modeldata.hpp"
#include "texenc.hpp"

#include <fmt/base.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {
using assetconv::TextureKind;
using sponge::scene::ModelData;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
namespace ktx2 = sponge::scene::ktx2;

bool encode(std::optional<ParsedImage>& image, const TextureKind kind) {
    if (!image) {
        return true;
    }

    image->ktx2 = assetconv::encode(*image, kind);
    if (image->ktx2.empty()) {
        return false;
    }

    image->pixels.clear();
    image->pixels.shrink_to_fit();
    return true;
}

bool encodeTextures(ModelData& data) {
    for (auto& mesh : data.meshes) {
        if (!encode(mesh.albedo, TextureKind::Color) ||
            !encode(mesh.normal, TextureKind::Normal) ||
            !encode(mesh.occlusion, TextureKind::Linear) ||
            !encode(mesh.emissive, TextureKind::Color) ||
            !encode(mesh.metallicRoughness, TextureKind::Linear)) {
            return false;
        }
    }
    return true;
}

ModelData import(const std::string& source) {
    const auto extension = std::filesystem::path(source).extension().string();
    if (extension != ".glb" && extension != ".gltf") {
        fmt::println(stderr, "assetconv: unsupported source format {}", source);
        return {};
    }
    return sponge::scene::gltf::parse(source);
}

bool writeFile(const std::string& path, const std::span<const uint8_t> bytes) {
    const std::filesystem::path out{ path };
    if (out.has_parent_path()) {
        std::filesystem::create_directories(out.parent_path());
    }

    std::ofstream file{ out, std::ios::binary | std::ios::trunc };
    if (!file) {
        fmt::println(stderr, "assetconv: unable to write {}", path);
        return false;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return file.good();
}

int convert(const std::string& source, const std::string& output) {
    auto data = import(source);
    if (data.meshes.empty()) {
        fmt::println(stderr, "assetconv: no meshes imported from {}", source);
        return 1;
    }

    if (!encodeTextures(data)) {
        return 1;
    }

    const auto bytes = sponge::scene::asset::write(data.meshes);
    if (!writeFile(output, bytes)) {
        return 1;
    }

    fmt::println("{} -> {} ({} meshes, {} bytes)", source, output,
                 data.meshes.size(), bytes.size());
    return 0;
}

// Re-imports the source and compares it against the baked file. This is the
// check that catches a layout or offset bug in the format itself.
int verify(const std::string& source, const std::string& output) {
    auto       expected = import(source);
    const auto actual   = sponge::scene::asset::read(output);

    if (expected.meshes.size() != actual.meshes.size()) {
        fmt::println(stderr, "FAIL {}: {} meshes baked, source has {}", output,
                     actual.meshes.size(), expected.meshes.size());
        return 1;
    }

    for (size_t i = 0; i < expected.meshes.size(); i++) {
        const auto& want = expected.meshes[i];
        const auto& got  = actual.meshes[i];

        if (want.vertices.size() != got.vertices.size() ||
            want.indices.size() != got.indices.size()) {
            fmt::println(stderr,
                         "FAIL {} mesh {}: {}/{} vertices, {}/{} indices",
                         output, i, got.vertices.size(), want.vertices.size(),
                         got.indices.size(), want.indices.size());
            return 1;
        }

        for (size_t v = 0; v < want.vertices.size(); v++) {
            if (want.vertices[v].position != got.vertices[v].position ||
                want.vertices[v].texCoords != got.vertices[v].texCoords ||
                want.vertices[v].normal != got.vertices[v].normal ||
                want.vertices[v].tangent != got.vertices[v].tangent) {
                fmt::println(stderr, "FAIL {} mesh {}: vertex {} differs",
                             output, i, v);
                return 1;
            }
        }

        if (want.indices != got.indices) {
            fmt::println(stderr, "FAIL {} mesh {}: indices differ", output, i);
            return 1;
        }

        if (want.albedo.has_value() != got.albedo.has_value()) {
            fmt::println(stderr, "FAIL {} mesh {}: albedo presence differs",
                         output, i);
            return 1;
        }

        if (got.albedo) {
            const auto image = ktx2::read(got.albedo->ktx2);
            if (image.width != want.albedo->width ||
                image.height != want.albedo->height) {
                fmt::println(stderr,
                             "FAIL {} mesh {}: albedo is {}x{}, source is "
                             "{}x{}",
                             output, i, image.width, image.height,
                             want.albedo->width, want.albedo->height);
                return 1;
            }
        }
    }

    fmt::println("OK {}: {} meshes match {}", output, actual.meshes.size(),
                 source);
    return 0;
}

// One image, BC7 with a full mip chain. For UI art that is too big to share
// a sprite sheet: a single large image forces the whole sheet up to the next
// power of two.
int convertTexture(const std::string& source, const std::string& output) {
    auto image = assetconv::loadImage(source);
    if (image.width == 0) {
        return 1;
    }

    const auto file = assetconv::encode(image, TextureKind::Color);
    if (file.empty() || !writeFile(output, file)) {
        return 1;
    }

    fmt::println("{} -> {} ({}x{}, {} bytes)", source, output, image.width,
                 image.height, file.size());
    return 0;
}

// Each argument is <sprite name>=<png path>. The name is what the engine
// looks the sprite up by, so it stays stable if the file moves.
int packAtlas(const std::string&                      output,
              const std::span<const std::string_view> args) {
    std::vector<assetconv::AtlasEntry> entries;
    entries.reserve(args.size());
    for (const auto arg : args) {
        const auto split = arg.find('=');
        if (split == std::string_view::npos) {
            fmt::println(stderr, "assetconv: expected <name>=<png>, got {}",
                         arg);
            return 2;
        }
        entries.emplace_back(assetconv::AtlasEntry{
            .name = std::string{ arg.substr(0, split) },
            .path = std::string{ arg.substr(split + 1) },
        });
    }

    return assetconv::packAtlas(entries, output) ? 0 : 1;
}
}  // namespace

int main(const int argc, char** argv) {
    // Log::init always adds a file sink, and a build-time tool has no
    // business writing one into the source or build tree.
    sponge::logging::Log::init(
        (std::filesystem::temp_directory_path() / "assetconv.log").string());

    assetconv::initEncoder();

    const std::vector<std::string_view> args{ argv + 1, argv + argc };
    if (args.size() == 3 && args[0] == "--verify") {
        return verify(std::string{ args[1] }, std::string{ args[2] });
    }
    if (args.size() >= 3 && args[0] == "--atlas") {
        return packAtlas(std::string{ args[1] }, std::span{ args }.subspan(2));
    }
    if (args.size() == 3 && args[0] == "--texture") {
        return convertTexture(std::string{ args[2] }, std::string{ args[1] });
    }
    if (args.size() == 2) {
        return convert(std::string{ args[0] }, std::string{ args[1] });
    }

    fmt::println(stderr,
                 "usage: assetconv [--verify] <source> <output.spnga>\n"
                 "       assetconv --atlas <output.ktx2> <name>=<png> ...\n"
                 "       assetconv --texture <output.ktx2> <input.png>");
    return 2;
}
