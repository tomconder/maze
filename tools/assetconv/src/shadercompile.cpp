#include "shadercompile.hpp"

#include "scene/shaderpack.hpp"

#include <fmt/base.h>
#include <slang-com-ptr.h>
#include <slang.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace assetconv {
namespace {

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

}  // namespace

std::optional<sponge::scene::shaderpack::Sources>
    compileShaders(const std::vector<ShaderEntry>& entries,
                   const bool                      lineDirectives) {
    Slang::ComPtr<slang::IGlobalSession> global;
    if (SLANG_FAILED(slang::createGlobalSession(global.writeRef()))) {
        fmt::println(stderr, "assetconv: unable to create a Slang session");
        return std::nullopt;
    }

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
        return std::nullopt;
    }

    // Several stages share a source file; load each module once.
    std::unordered_map<std::string, slang::IModule*> modules;
    sponge::scene::shaderpack::Sources               sources;

    for (const auto& entry : entries) {
        auto& module = modules[entry.path];
        if (module == nullptr) {
            const auto text = readText(entry.path);
            if (!text) {
                fmt::println(stderr, "assetconv: unable to read {}",
                             entry.path);
                return std::nullopt;
            }
            // #include resolves against this path.
            const auto moduleName =
                std::filesystem::path(entry.path).stem().string();
            Slang::ComPtr<slang::IBlob> diagnostics;
            module = session->loadModuleFromSourceString(
                moduleName.c_str(), entry.path.c_str(), text->c_str(),
                diagnostics.writeRef());
            printDiagnostics(diagnostics);
            if (module == nullptr) {
                return std::nullopt;
            }
        }

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
            fmt::println(stderr, "assetconv: unable to compile {} [{}]",
                         entry.path, entry.entryPoint);
            return std::nullopt;
        }
        printDiagnostics(diagnostics);

        const auto [it, inserted] = sources.try_emplace(
            entry.name, static_cast<const char*>(code->getBufferPointer()),
            code->getBufferSize());
        if (!inserted) {
            fmt::println(stderr, "assetconv: shader name {} is used twice",
                         entry.name);
            return std::nullopt;
        }
    }

    return sources;
}

}  // namespace assetconv
