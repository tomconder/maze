#include "platform/opengl/renderer/resourcemanager.hpp"

namespace sponge::platform::opengl::renderer {
ResourceHandler<scene::Font, scene::FontCreateInfo>
                                            ResourceManager::fontHandler;
ResourceHandler<Texture, TextureCreateInfo> ResourceManager::textureHandler;
ResourceHandler<scene::Model, scene::ModelCreateInfo>
                                          ResourceManager::modelHandler;
ResourceHandler<Shader, ShaderCreateInfo> ResourceManager::shaderHandler;
}  // namespace sponge::platform::opengl::renderer
