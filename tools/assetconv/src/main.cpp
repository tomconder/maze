// Asset converter. Reads one source model and writes the baked .spnga the
// engine loads. CMake drives one invocation per entry in assets/manifest.json.
//
// Usage: assetconv <source> <output.spnga>
//        assetconv --verify <source> <output.spnga>

#include "logging/log.hpp"
#include "scene/assetformat.hpp"
#include "scene/gltfimport.hpp"
#include "scene/ktx2.hpp"
#include "scene/modeldata.hpp"

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
using sponge::scene::ModelData;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
namespace ktx2 = sponge::scene::ktx2;

// Uncompressed formats only. BC7/BC5 encoding is the next step; until then
// the baked file reproduces exactly what the runtime uploaded before, so a
// bake cannot change how a model looks.
ktx2::Format formatFor(const uint32_t bytesPerPixel) {
    switch (bytesPerPixel) {
        case 1:
            return ktx2::formatR8Unorm;
        case 2:
            return ktx2::formatR8G8Unorm;
        case 3:
            return ktx2::formatR8G8B8Unorm;
        case 4:
            return ktx2::formatR8G8B8A8Unorm;
        default:
            return ktx2::formatUndefined;
    }
}

bool encode(std::optional<ParsedImage>& image) {
    if (!image) {
        return true;
    }

    const auto format = formatFor(image->bytesPerPixel);
    if (format == ktx2::formatUndefined) {
        SPONGE_ERROR("Unsupported channel count {} in {}", image->bytesPerPixel,
                     image->name);
        return false;
    }

    image->ktx2 =
        ktx2::write(format, image->width, image->height, image->pixels);
    image->pixels.clear();
    image->pixels.shrink_to_fit();
    return true;
}

bool encodeTextures(ModelData& data) {
    for (auto& mesh : data.meshes) {
        if (!encode(mesh.albedo) || !encode(mesh.normal) ||
            !encode(mesh.occlusion) || !encode(mesh.emissive) ||
            !encode(mesh.metallicRoughness)) {
            return false;
        }
    }
    return true;
}

ModelData import(const std::string& source) {
    const auto extension = std::filesystem::path(source).extension().string();
    if (extension != ".glb" && extension != ".gltf") {
        SPONGE_ERROR("Unsupported source format {}", source);
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
        SPONGE_ERROR("Unable to write {}", path);
        return false;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return file.good();
}

int convert(const std::string& source, const std::string& output) {
    auto data = import(source);
    if (data.meshes.empty()) {
        SPONGE_ERROR("No meshes imported from {}", source);
        return 1;
    }

    if (!encodeTextures(data)) {
        return 1;
    }

    const auto bytes = sponge::scene::asset::write(data.meshes);
    if (!writeFile(output, bytes)) {
        return 1;
    }

    std::printf("%s -> %s (%zu meshes, %zu bytes)\n", source.c_str(),
                output.c_str(), data.meshes.size(), bytes.size());
    return 0;
}

// Re-imports the source and compares it against the baked file. This is the
// check that catches a layout or offset bug in the format itself.
int verify(const std::string& source, const std::string& output) {
    auto       expected = import(source);
    const auto actual   = sponge::scene::asset::read(output);

    if (expected.meshes.size() != actual.meshes.size()) {
        std::printf("FAIL %s: %zu meshes baked, source has %zu\n",
                    output.c_str(), actual.meshes.size(),
                    expected.meshes.size());
        return 1;
    }

    for (size_t i = 0; i < expected.meshes.size(); i++) {
        const auto& want = expected.meshes[i];
        const auto& got  = actual.meshes[i];

        if (want.vertices.size() != got.vertices.size() ||
            want.indices.size() != got.indices.size()) {
            std::printf("FAIL %s mesh %zu: %zu/%zu vertices, %zu/%zu indices\n",
                        output.c_str(), i, got.vertices.size(),
                        want.vertices.size(), got.indices.size(),
                        want.indices.size());
            return 1;
        }

        for (size_t v = 0; v < want.vertices.size(); v++) {
            if (want.vertices[v].position != got.vertices[v].position ||
                want.vertices[v].texCoords != got.vertices[v].texCoords ||
                want.vertices[v].normal != got.vertices[v].normal ||
                want.vertices[v].tangent != got.vertices[v].tangent) {
                std::printf("FAIL %s mesh %zu: vertex %zu differs\n",
                            output.c_str(), i, v);
                return 1;
            }
        }

        if (want.indices != got.indices) {
            std::printf("FAIL %s mesh %zu: indices differ\n", output.c_str(),
                        i);
            return 1;
        }

        if (want.albedo.has_value() != got.albedo.has_value()) {
            std::printf("FAIL %s mesh %zu: albedo presence differs\n",
                        output.c_str(), i);
            return 1;
        }

        if (got.albedo) {
            const auto image = ktx2::read(got.albedo->ktx2);
            if (image.width != want.albedo->width ||
                image.height != want.albedo->height) {
                std::printf("FAIL %s mesh %zu: albedo is %ux%u, source is "
                            "%ux%u\n",
                            output.c_str(), i, image.width, image.height,
                            want.albedo->width, want.albedo->height);
                return 1;
            }
        }
    }

    std::printf("OK %s: %zu meshes match %s\n", output.c_str(),
                actual.meshes.size(), source.c_str());
    return 0;
}
}  // namespace

int main(const int argc, char** argv) {
    // Log::init always adds a file sink, and a build-time tool has no
    // business writing one into the source or build tree.
    sponge::logging::Log::init(
        (std::filesystem::temp_directory_path() / "assetconv.log").string());

    const std::vector<std::string_view> args{ argv + 1, argv + argc };
    if (args.size() == 3 && args[0] == "--verify") {
        return verify(std::string{ args[1] }, std::string{ args[2] });
    }
    if (args.size() == 2) {
        return convert(std::string{ args[0] }, std::string{ args[1] });
    }

    std::printf("usage: assetconv [--verify] <source> <output.spnga>\n");
    return 2;
}
