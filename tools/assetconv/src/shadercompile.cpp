#include "shadercompile.hpp"

#include "shaderpack.hpp"

#include <fmt/base.h>
#include <slang-com-ptr.h>
#include <slang.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace assetconv {
namespace {

// Slang creates global sessions one at a time, about 0.2s each, and each
// session holds about 170MB. Past a few threads that serial start-up costs
// more than the extra threads save. For the 25 stages in the manifest, 3
// threads bake in 1.3s, 4 in 1.4s and 8 in 1.8s. Raise this when the shaders
// grow enough that compiling outweighs start-up.
constexpr unsigned maxSessions = 3;

void printDiagnostics(slang::IBlob* diagnostics) {
    if (diagnostics != nullptr && diagnostics->getBufferSize() > 0) {
        fmt::println(stderr, "{}",
                     std::string_view{ static_cast<const char*>(
                                           diagnostics->getBufferPointer()),
                                       diagnostics->getBufferSize() });
    }
}

std::optional<std::string> readText(const std::string& path) {
    std::ifstream file{ path, std::ios::binary };
    if (!file) {
        return std::nullopt;
    }
    return std::string{ std::istreambuf_iterator<char>{ file }, {} };
}

Slang::ComPtr<slang::ISession> createSession(slang::IGlobalSession* global,
                                             const bool lineDirectives) {
    slang::TargetDesc target;
    target.format  = SLANG_GLSL;
    target.profile = global->findProfile("glsl_450");
    // Release drops #line so shipped GLSL carries no source paths.
    target.lineDirectiveMode = lineDirectives ?
                                   SLANG_LINE_DIRECTIVE_MODE_DEFAULT :
                                   SLANG_LINE_DIRECTIVE_MODE_NONE;

    slang::SessionDesc sessionDesc;
    sessionDesc.targets                 = &target;
    sessionDesc.targetCount             = 1;
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(global->createSession(sessionDesc, session.writeRef()))) {
        fmt::println(stderr, "assetconv: unable to create a Slang session");
    }
    return session;
}

// Compiles one entry point of a loaded module. Returns nothing, after printing
// Slang's diagnostics, on failure.
std::optional<std::string> compileEntry(slang::ISession*   session,
                                        slang::IModule*    module,
                                        const ShaderEntry& entry) {
    Slang::ComPtr<slang::IEntryPoint> entryPoint;
    if (SLANG_FAILED(module->findEntryPointByName(entry.entryPoint.c_str(),
                                                  entryPoint.writeRef()))) {
        fmt::println(stderr, "assetconv: no entry point {} in {}",
                     entry.entryPoint, entry.path);
        return std::nullopt;
    }

    const std::array<slang::IComponentType*, 2> parts{ module,
                                                       entryPoint.get() };
    Slang::ComPtr<slang::IComponentType>        composed;
    Slang::ComPtr<slang::IComponentType>        linked;
    Slang::ComPtr<slang::IBlob>                 code;
    Slang::ComPtr<slang::IBlob>                 diagnostics;

    if (SLANG_FAILED(session->createCompositeComponentType(
            parts.data(), parts.size(), composed.writeRef(),
            diagnostics.writeRef())) ||
        SLANG_FAILED(
            composed->link(linked.writeRef(), diagnostics.writeRef())) ||
        SLANG_FAILED(linked->getEntryPointCode(0, 0, code.writeRef(),
                                               diagnostics.writeRef()))) {
        printDiagnostics(diagnostics);
        fmt::println(stderr, "assetconv: unable to compile {} [{}]", entry.path,
                     entry.entryPoint);
        return std::nullopt;
    }
    printDiagnostics(diagnostics);

    return std::string{ static_cast<const char*>(code->getBufferPointer()),
                        code->getBufferSize() };
}

}  // namespace

std::optional<sponge::scene::shaderpack::Sources>
    compileShaders(const std::vector<ShaderEntry>& entries,
                   const bool lineDirectives, const unsigned threads) {
    // Several stages share a source file, so the unit of work is a module:
    // load it once, then compile each of its stages.
    std::vector<std::string>                                  paths;
    std::unordered_map<std::string, std::vector<std::size_t>> stagesOf;
    for (std::size_t i = 0; i < entries.size(); i++) {
        auto& stages = stagesOf[entries[i].path];
        if (stages.empty()) {
            paths.push_back(entries[i].path);
        }
        stages.push_back(i);
    }

    // A Slang session and what it loads belong to one thread, so each worker
    // has its own.
    std::vector<std::optional<std::string>> code(entries.size());
    std::atomic<std::size_t>                next{ 0 };
    std::atomic<bool>                       failed{ false };
    const auto                              work = [&] {
        Slang::ComPtr<slang::IGlobalSession> global;
        if (SLANG_FAILED(slang::createGlobalSession(global.writeRef()))) {
            fmt::println(stderr, "assetconv: unable to create a Slang session");
            failed = true;
            return;
        }
        const auto session = createSession(global, lineDirectives);
        if (!session) {
            failed = true;
            return;
        }

        for (auto m = next++; m < paths.size() && !failed; m = next++) {
            const auto& path = paths[m];
            const auto  text = readText(path);
            if (!text) {
                fmt::println(stderr, "assetconv: unable to read {}", path);
                failed = true;
                return;
            }
            // #include resolves against this path.
            const auto moduleName = std::filesystem::path(path).stem().string();
            Slang::ComPtr<slang::IBlob> diagnostics;
            auto* module = session->loadModuleFromSourceString(
                moduleName.c_str(), path.c_str(), text->c_str(),
                diagnostics.writeRef());
            printDiagnostics(diagnostics);
            if (module == nullptr) {
                failed = true;
                return;
            }

            for (const auto i : stagesOf.at(path)) {
                code[i] = compileEntry(session, module, entries[i]);
                if (!code[i]) {
                    failed = true;
                    return;
                }
            }
        }
    };

    const auto workers =
        std::min<std::size_t>({ threads, maxSessions, paths.size() });
    std::vector<std::jthread> pool;
    for (std::size_t i = 0; i < workers; i++) {
        pool.emplace_back(work);
    }
    pool.clear();  // joins

    if (failed) {
        return std::nullopt;
    }

    // Inserted in manifest order, as a serial compile would.
    sponge::scene::shaderpack::Sources sources;
    for (std::size_t i = 0; i < entries.size(); i++) {
        const auto [it, inserted] =
            sources.try_emplace(entries[i].name, std::move(*code[i]));
        if (!inserted) {
            fmt::println(stderr, "assetconv: shader name {} is used twice",
                         entries[i].name);
            return std::nullopt;
        }
    }
    return sources;
}

}  // namespace assetconv
