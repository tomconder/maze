#pragma once

#include "platform/opengl/renderer/shader.hpp"
#include "platform/opengl/renderer/texture.hpp"
#include "platform/opengl/scene/bitmapfont.hpp"
#include "platform/opengl/scene/model.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace sponge::platform::opengl::renderer {

// Asset cache. create*() returns a shared_ptr handle; callers retain it and
// reuse it directly — there is no by-name lookup at runtime.
template <typename T, typename C>
class AssetHandler final {
public:
    std::shared_ptr<T> load(const C& createInfo) {
        assert(!createInfo.name.empty());

        const auto [it, inserted] = assets.try_emplace(createInfo.name);
        if (inserted) {
            it->second = std::make_shared<T>(createInfo);
        }
        return it->second;
    }

    // Registers an asset built outside load()'s single-shot make_shared path
    // (e.g. constructed from data parsed on a background thread).
    void insert(const std::string& name, std::shared_ptr<T> asset) {
        assets[name] = std::move(asset);
    }

    const auto& getAssets() const {
        return assets;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<T>> assets;
};

class AssetManager {
public:
    static std::shared_ptr<scene::BitmapFont>
        createFont(const scene::FontCreateInfo& createInfo) {
        return fontHandler.load(createInfo);
    }
    static auto& getFonts() {
        return fontHandler.getAssets();
    }

    // Registers a Model built via Model(ModelData&&) (background-parsed), so it
    // shows up in getModels() (the ImGui debug table).
    static void registerModel(const std::string&            name,
                              std::shared_ptr<scene::Model> model) {
        modelHandler.insert(name, std::move(model));
    }

    static auto& getModels() {
        return modelHandler.getAssets();
    }

    static std::shared_ptr<Shader>
        createShader(const ShaderCreateInfo& createInfo) {
        return shaderHandler.load(createInfo);
    }
    static auto& getShaders() {
        return shaderHandler.getAssets();
    }

    static std::shared_ptr<Texture>
        createTexture(const TextureCreateInfo& createInfo) {
        return textureHandler.load(createInfo);
    }
    static auto& getTextures() {
        return textureHandler.getAssets();
    }

private:
    AssetManager() = default;

    inline static AssetHandler<Shader, ShaderCreateInfo>   shaderHandler;
    inline static AssetHandler<Texture, TextureCreateInfo> textureHandler;
    inline static AssetHandler<scene::BitmapFont, scene::FontCreateInfo>
        fontHandler;
    inline static AssetHandler<scene::Model, scene::ModelCreateInfo>
        modelHandler;
};
}  // namespace sponge::platform::opengl::renderer
