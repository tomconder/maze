// Asset converter. Bakes the models, textures, atlases and shaders listed in
// assets/manifest.json to the formats the engine loads.
//
// Usage: assetconv --manifest <manifest.json> <output dir>
//                  [--no-line-directives]
//        assetconv --verify <source> <output.spnga>

#include "atlas.hpp"
#include "gltfimport.hpp"
#include "logging/log.hpp"
#include "meshopt.hpp"
#include "objimport.hpp"
#include "scene/assetformat.hpp"
#include "scene/ktx2.hpp"
#include "scene/modeldata.hpp"
#include "scene/shaderpack.hpp"
#include "shadercompile.hpp"
#include "texenc.hpp"

#include <fmt/base.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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
    if (extension == ".glb" || extension == ".gltf") {
        return assetconv::gltf::parse(source);
    }
    if (extension == ".obj") {
        return assetconv::obj::parse(source);
    }
    fmt::println(stderr, "assetconv: unsupported source format {}", source);
    return {};
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

    // Vertex cache and fetch order used to be computed on every load. It is
    // deterministic, so the baked file carries the result instead.
    for (auto& mesh : data.meshes) {
        assetconv::optimizeMesh(mesh);
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
    auto expected = import(source);
    // The bake optimizes before writing, so the comparison has to as well.
    // This checks the round trip through the container, not the optimizer.
    for (auto& mesh : expected.meshes) {
        assetconv::optimizeMesh(mesh);
    }
    const auto actual = sponge::scene::asset::read(output);

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

int packShaders(const std::string&                         output,
                const std::vector<assetconv::ShaderEntry>& entries,
                const bool                                 lineDirectives) {
    const auto sources = assetconv::compileShaders(entries, lineDirectives);
    if (!sources) {
        return 1;
    }

    const auto bytes = sponge::scene::shaderpack::write(*sources);
    if (!writeFile(output, bytes)) {
        return 1;
    }

    fmt::println("{} shaders -> {} ({} bytes)", sources->size(), output,
                 bytes.size());
    return 0;
}

int packAtlas(const std::vector<assetconv::AtlasEntry>& entries,
              const std::string&                        output) {
    const auto file = assetconv::packAtlas(entries);
    if (file.empty() || !writeFile(output, file)) {
        return 1;
    }

    const auto image = ktx2::read(file);
    fmt::println("{} sprites -> {} ({}x{}, {} bytes)", entries.size(), output,
                 image.width, image.height, file.size());
    return 0;
}

using Json   = nlohmann::ordered_json;
namespace fs = std::filesystem;

// Current when newer than every input. A missing file on either side is stale.
bool upToDate(const fs::path& output, const std::vector<fs::path>& inputs) {
    std::error_code ec;
    const auto      built = fs::last_write_time(output, ec);
    if (ec) {
        return false;
    }
    for (const auto& input : inputs) {
        const auto changed = fs::last_write_time(input, ec);
        if (ec || changed > built) {
            return false;
        }
    }
    return true;
}

// Sources are relative to the manifest's folder, outputs to outputDir.
int bakeManifest(const std::string& manifestPath, const std::string& outputDir,
                 const bool lineDirectives, const fs::path& converter) {
    const auto sourceRoot = fs::path(manifestPath).parent_path();
    const auto outputRoot = fs::path(outputDir);
    const auto source     = [&](const std::string& path) {
        return (sourceRoot / path).generic_string();
    };

    // Every output also depends on the manifest and on the converter, which
    // stands in for the format headers compiled into it.
    const auto bake = [&](const std::string& name, std::vector<fs::path> inputs,
                          const auto& run) {
        const auto output = (outputRoot / name).generic_string();
        inputs.emplace_back(manifestPath);
        inputs.emplace_back(converter);
        if (upToDate(output, inputs) || run(output)) {
            return true;
        }
        // A partial file would look current on the next run.
        std::error_code ec;
        fs::remove(output, ec);
        return false;
    };

    try {
        std::ifstream file{ manifestPath };
        const auto    manifest = Json::parse(file);

        for (const auto& model : manifest.value("models", Json::array())) {
            const auto from = source(model.at("source").get<std::string>());
            if (!bake(model.at("output").get<std::string>(), { from },
                      [&](const std::string& to) {
                          return convert(from, to) == 0;
                      })) {
                return 1;
            }
        }

        for (const auto& texture : manifest.value("textures", Json::array())) {
            const auto from = source(texture.at("source").get<std::string>());
            if (!bake(texture.at("output").get<std::string>(), { from },
                      [&](const std::string& to) {
                          return convertTexture(from, to) == 0;
                      })) {
                return 1;
            }
        }

        // The sprite name is what the engine looks up, so it stays stable if
        // the file moves.
        for (const auto& atlas : manifest.value("atlases", Json::array())) {
            std::vector<assetconv::AtlasEntry> entries;
            std::vector<fs::path>              inputs;
            for (const auto& [sprite, path] : atlas.at("sprites").items()) {
                entries.push_back({ .name = sprite,
                                    .path = source(path.get<std::string>()) });
                inputs.emplace_back(entries.back().path);
            }
            if (!bake(atlas.at("output").get<std::string>(), inputs,
                      [&](const std::string& to) {
                          return packAtlas(entries, to) == 0;
                      })) {
                return 1;
            }
        }

        // Each stage is <slang path>:<entry point>, keyed by the name the
        // engine asks for.
        if (manifest.contains("shaders")) {
            const auto& shaders = manifest.at("shaders");
            std::vector<assetconv::ShaderEntry> entries;
            for (const auto& [stage, value] : shaders.at("stages").items()) {
                const auto text  = value.get<std::string>();
                const auto colon = text.rfind(':');
                if (colon == std::string::npos) {
                    fmt::println(stderr,
                                 "assetconv: stage {} is not <slang>:<entry>, "
                                 "got {}",
                                 stage, text);
                    return 2;
                }
                entries.push_back({ .name       = stage,
                                    .path       = source(text.substr(0, colon)),
                                    .entryPoint = text.substr(colon + 1) });
            }

            // Includes are not listed anywhere, so every file under a stage's
            // folder counts as an input.
            std::set<fs::path> folders;
            for (const auto& entry : entries) {
                folders.insert(fs::path(entry.path).parent_path());
            }
            std::vector<fs::path> inputs;
            for (const auto& folder : folders) {
                for (const auto& item :
                     fs::recursive_directory_iterator(folder)) {
                    if (item.is_regular_file()) {
                        inputs.push_back(item.path());
                    }
                }
            }

            if (!bake(shaders.at("output").get<std::string>(), inputs,
                      [&](const std::string& to) {
                          return packShaders(to, entries, lineDirectives) == 0;
                      })) {
                return 1;
            }
        }
    } catch (const std::exception& e) {
        fmt::println(stderr, "assetconv: {}: {}", manifestPath, e.what());
        return 2;
    }
    return 0;
}
}  // namespace

int main(const int argc, char** argv) {
    // Log::init always adds a file sink, and a build-time tool has no
    // business writing one into the source or build tree.
    sponge::logging::Log::init(
        (std::filesystem::temp_directory_path() / "assetconv.log").string());

    assetconv::initEncoder();

    const std::vector<std::string_view> args{ argv + 1, argv + argc };
    if ((args.size() == 3 || args.size() == 4) && args[0] == "--manifest") {
        if (args.size() == 4 && args[3] != "--no-line-directives") {
            fmt::println(stderr, "assetconv: unknown option {}", args[3]);
            return 2;
        }
        return bakeManifest(std::string{ args[1] }, std::string{ args[2] },
                            args.size() == 3, argv[0]);
    }
    if (args.size() == 3 && args[0] == "--verify") {
        return verify(std::string{ args[1] }, std::string{ args[2] });
    }

    fmt::println(stderr,
                 "usage: assetconv --manifest <manifest.json> <output dir> "
                 "[--no-line-directives]\n"
                 "       assetconv --verify <source> <output.spnga>");
    return 2;
}
