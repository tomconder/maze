#include "platform/opengl/renderer/assetmanager.hpp"

namespace sponge::platform::opengl::renderer {
AssetHandler<scene::MSDFFont, scene::FontCreateInfo> AssetManager::fontHandler;
AssetHandler<Texture, TextureCreateInfo>           AssetManager::textureHandler;
AssetHandler<scene::Model, scene::ModelCreateInfo> AssetManager::modelHandler;
AssetHandler<Shader, ShaderCreateInfo>             AssetManager::shaderHandler;
}  // namespace sponge::platform::opengl::renderer
