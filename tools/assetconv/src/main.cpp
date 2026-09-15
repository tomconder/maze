// Asset converter. Bakes the models, textures, fonts, atlases and shaders
// listed in assets/manifest.json to the formats the engine loads, and joins
// the third-party licenses into one notices file.
//
// Usage: assetconv [--threads <n>] --manifest <manifest.json> <output dir>
//                  [--no-line-directives]
//                  [--notices <file> [--license <name>=<path>]...]
//        assetconv [--threads <n>] --verify <source> <output.spnga>

#include "assetformat.hpp"
#include "atlas.hpp"
#include "fontbake.hpp"
#include "gltfimport.hpp"
#include "ktx2.hpp"
#include "meshopt.hpp"
#include "modeldata.hpp"
#include "objimport.hpp"
#include "readbytes.hpp"
#include "shadercompile.hpp"
#include "shaderpack.hpp"
#include "texenc.hpp"

#include <fmt/base.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {
using assetconv::TextureKind;
using sponge::scene::ModelData;
using sponge::scene::ParsedImage;
using sponge::scene::ParsedMesh;
namespace ktx2 = sponge::scene::ktx2;

// A mesh's texture slots with the kind each wants, in the writer's slot order.
std::array<std::pair<std::optional<uint32_t>, TextureKind>, 5>
    slotsOf(const ParsedMesh& mesh) {
    return { { { mesh.albedo, TextureKind::Color },
               { mesh.normal, TextureKind::Normal },
               { mesh.occlusion, TextureKind::Linear },
               { mesh.emissive, TextureKind::Color },
               { mesh.metallicRoughness, TextureKind::Linear } } };
}

bool encodeTextures(ModelData& data) {
    // The file holds one encoding per image, so the first slot that uses an
    // image picks its kind.
    std::vector<std::optional<TextureKind>> kinds(data.images.size());
    for (const auto& mesh : data.meshes) {
        for (const auto& [image, kind] : slotsOf(mesh)) {
            if (!image) {
                continue;
            }
            auto& chosen = kinds[*image];
            if (!chosen) {
                chosen = kind;
            } else if (*chosen != kind) {
                fmt::println(stderr,
                             "assetconv: {} is used as more than one kind of "
                             "texture; encoding it for its first use",
                             data.images[*image].name);
            }
        }
    }

    for (size_t i = 0; i < data.images.size(); i++) {
        if (!kinds[i]) {
            continue;
        }
        auto& image = data.images[i];
        image.ktx2  = assetconv::encode(image, *kinds[i]);
        if (image.ktx2.empty()) {
            return false;
        }
        image.pixels.clear();
        image.pixels.shrink_to_fit();
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

    const auto bytes = sponge::scene::asset::write(data);
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
    std::string error;
    const auto  actual = sponge::scene::asset::read(output, error);
    if (!error.empty()) {
        fmt::println(stderr, "FAIL {}", error);
        return 1;
    }

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
            const auto& sourceImage = expected.images[*want.albedo];
            const auto  image =
                ktx2::read(actual.images[*got.albedo].ktx2, error);
            if (!error.empty()) {
                fmt::println(stderr, "FAIL {} mesh {}: {}", output, i, error);
                return 1;
            }
            if (image.width != sourceImage.width ||
                image.height != sourceImage.height) {
                fmt::println(stderr,
                             "FAIL {} mesh {}: albedo is {}x{}, source is "
                             "{}x{}",
                             output, i, image.width, image.height,
                             sourceImage.width, sourceImage.height);
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
                const bool lineDirectives, const unsigned threads) {
    const auto sources =
        assetconv::compileShaders(entries, lineDirectives, threads);
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

int convertFont(const std::string& source, const std::string& output,
                const std::vector<uint32_t>& sizes) {
    const auto file = assetconv::bakeFont(source, sizes);
    if (file.empty() || !writeFile(output, file)) {
        return 1;
    }

    fmt::println("{} -> {} ({} sizes, {} bytes)", source, output, sizes.size(),
                 file.size());
    return 0;
}

int packAtlas(const std::vector<assetconv::AtlasEntry>& entries,
              const std::string&                        output) {
    const auto file = assetconv::packAtlas(entries);
    if (file.empty() || !writeFile(output, file)) {
        return 1;
    }

    std::string error;
    const auto  image = ktx2::read(file, error);
    if (!error.empty()) {
        fmt::println(stderr, "assetconv: {}: {}", output, error);
        return 1;
    }
    fmt::println("{} sprites -> {} ({}x{}, {} bytes)", entries.size(), output,
                 image.width, image.height, file.size());
    return 0;
}

using Json   = nlohmann::ordered_json;
namespace fs = std::filesystem;

// License name to license files.
using Licenses = std::map<std::string, std::vector<std::string>>;

// One notices file with a section per license. A section with several files
// lists each under its file name, as vcpkg_install_copyright does. Written on
// every run, but only when the text changes: the list comes partly from the
// command line, which has no timestamp.
bool writeNotices(const std::string& output, const Licenses& licenses) {
    const std::string rule(80, '=');
    std::string       text;
    for (const auto& [name, files] : licenses) {
        text += rule + "\n" + name + "\n" + rule + "\n\n";
        for (const auto& path : files) {
            const auto bytes = sponge::scene::readBytes(path);
            if (bytes.empty()) {
                fmt::println(stderr, "assetconv: unable to read {}", path);
                return false;
            }
            std::string contents{ bytes.begin(), bytes.end() };
            std::erase(contents, '\r');
            if (files.size() > 1) {
                text += fs::path(path).filename().string() + ":\n\n";
            }
            text += contents + "\n\n";
        }
    }

    const auto        current = sponge::scene::readBytes(output);
    const std::string written(current.begin(), current.end());
    if (written == text) {
        return true;
    }
    if (!writeFile(output, { reinterpret_cast<const uint8_t*>(text.data()),
                             text.size() })) {
        return false;
    }
    fmt::println("{} licenses -> {} ({} bytes)", licenses.size(), output,
                 text.size());
    return true;
}

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
                 const bool lineDirectives, const unsigned threads,
                 const fs::path& converter, const std::string& notices,
                 Licenses licenses) {
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

        for (const auto& font : manifest.value("fonts", Json::array())) {
            const auto from  = source(font.at("source").get<std::string>());
            const auto sizes = font.at("sizes").get<std::vector<uint32_t>>();
            if (!bake(font.at("output").get<std::string>(), { from },
                      [&](const std::string& to) {
                          return convertFont(from, to, sizes) == 0;
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
                          return packShaders(to, entries, lineDirectives,
                                             threads) == 0;
                      })) {
                return 1;
            }
        }

        // Asset licenses come from the manifest, code licenses from the
        // command line: only the build knows what the game links.
        if (!notices.empty()) {
            for (const auto& license :
                 manifest.value("licenses", Json::array())) {
                auto& files = licenses[license.at("name").get<std::string>()];
                for (const auto& path : license.at("files")) {
                    files.push_back(source(path.get<std::string>()));
                }
            }
            if (!writeNotices(notices, licenses)) {
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
    std::vector<std::string_view> args{ argv + 1, argv + argc };

    // 0: one thread per hardware thread.
    unsigned threads = 0;
    if (args.size() >= 2 && args[0] == "--threads") {
        const auto value = args[1];
        const auto [end, error] =
            std::from_chars(value.data(), value.data() + value.size(), threads);
        if (error != std::errc() || end != value.data() + value.size() ||
            threads == 0) {
            fmt::println(stderr, "assetconv: --threads needs a number above 0");
            return 2;
        }
        args.erase(args.begin(), args.begin() + 2);
    }
    if (threads == 0) {
        threads = std::max(std::thread::hardware_concurrency(), 1U);
    }
    assetconv::initEncoder(threads);

    if (args.size() >= 3 && args[0] == "--manifest") {
        bool        lineDirectives = true;
        std::string notices;
        Licenses    licenses;
        for (size_t i = 3; i < args.size(); i++) {
            if (args[i] == "--no-line-directives") {
                lineDirectives = false;
            } else if (args[i] == "--notices" && i + 1 < args.size()) {
                notices = args[++i];
            } else if (args[i] == "--license" && i + 1 < args.size() &&
                       args[i + 1].find('=') != std::string_view::npos) {
                const auto value  = args[++i];
                const auto equals = value.find('=');
                licenses[std::string{ value.substr(0, equals) }].emplace_back(
                    value.substr(equals + 1));
            } else {
                fmt::println(stderr, "assetconv: unknown option {}", args[i]);
                return 2;
            }
        }
        return bakeManifest(std::string{ args[1] }, std::string{ args[2] },
                            lineDirectives, threads, argv[0], notices,
                            std::move(licenses));
    }
    if (args.size() == 3 && args[0] == "--verify") {
        return verify(std::string{ args[1] }, std::string{ args[2] });
    }

    fmt::println(stderr,
                 "usage: assetconv [--threads <n>] --manifest <manifest.json> "
                 "<output dir> [--no-line-directives]\n"
                 "           [--notices <file> [--license <name>=<path>]...]\n"
                 "       assetconv [--threads <n>] --verify <source> "
                 "<output.spnga>");
    return 2;
}
