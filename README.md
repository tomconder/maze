# Maze

A game engine featuring a nice walk through a maze.

![work in progress](docs/docs/static/img/workinprogress.png "work in progress")

## Features

* Platform portability: OpenGL and C++ for Windows and Linux
* Physically based rendering (PBR)
* Cook-Torrance microfacet specular BRDF
* glTF 2.0 materials: albedo, normal, occlusion, emissive and metallic-roughness maps, with KHR_texture_transform
* Point lights and directional light
* Clustered (volume-tiled) light culling
* Exponential variance shadow maps (EVSM) with Dual Kawase blur
* Reinhard tone mapping
* Physically based bloom
* Selectable anti-aliasing: FXAA or temporal (TAA)
* Shaders written in Slang, compiled at build time into one shader pack
* Build-time asset converter: glTF 2.0 and Wavefront OBJ models baked to an engine format, so the game links no asset parser
* BC7 and BC5 texture compression with mipmaps in KTX2, encoded on every core
* UI icons packed into a sprite atlas
* LCD subpixel text with subpixel positioning, rasterized with FreeType and shaped with HarfBuzz at build time
* Flexbox-based UI layout with Yoga
* Audio playback with miniaudio
* Performance profiling with Tracy
* Gamepad support
* Dear ImGui debug and options UI
* Third-party license notices assembled at build time and shipped with the game

## Project Organization

```
assets/          # game assets
docs/            # documentation
game/            # source files for maze
sponge/          # source files for sponge game engine
tools/           # tools and utility scripts, including the assetconv asset converter
```

## Getting Started

For instructions on how to build and run the application, please refer to
the [full documentation](https://tomconder.github.io/maze/)
