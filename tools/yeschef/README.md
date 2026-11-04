# yeschef

Rust workspace for the asset pipeline. It will replace `tools/assetconv`. It holds two crates, `format` and `yeschef`.

## format

`format` (`yeschef-format`) defines the byte layout of the baked files: the `.spnga` model container and the `shaders.spnga` shader pack. The structs are `repr(C)`, and compile-time asserts pin their sizes.

cbindgen turns the structs into `sponge/format/scene/src/assetlayout.generated.hpp`. The engine and `assetconv` include that header, so they use the layout Rust defines. The header is checked in, and the C++ build does not run Rust.

To change the layout:

1. Edit `format/src/lib.rs`. Bump `ASSET_VERSION` or `SHADERPACK_VERSION` if the layout changes.
2. Run `UPDATE_LAYOUT_HEADER=1 cargo test` here to rewrite the header.
3. Rebuild the C++ side. The asserts in `sponge/format/scene/src/assetformat.hpp` fail if a hand-written glm twin no longer matches.

`cargo test` fails when the checked-in header differs from what cbindgen makes.

To check the structs against real baked files, bake with `assetconv`, then run:

```
YESCHEF_BAKED_DIR=<bake output dir> cargo test
```

Without the variable, that test passes without checking anything.

Not covered yet: the font tables (`Glyph` in `font.hpp`) and the KTX2 files. `yeschef/src/font.rs` writes the font tables by hand to match `font.cpp`, and the glyph atlas is a plain KTX2 file.

## yeschef

The converter binary. It takes the command line of `assetconv`: `--manifest <manifest.yaml> <output dir>`, `--no-line-directives`, `--no-debug-assets`, `--notices <file> [--license NAME=PATH]...` and `--verify`. `--threads` is accepted and ignored for now.

Done: manifest parsing, up-to-date checks, orphan deletion and the notices file. The notices output is byte-identical to `assetconv`.

Shader pack: done. `yeschef/src/shaders.rs` compiles the manifest's Slang stages to GLSL 450 and writes the pack with `format/src/shaderpack.rs`. Against `assetconv` the pack is byte-identical, with and without line directives, when both use the same Slang version.

Environment maps: done. `yeschef/src/hdr.rs` bakes Radiance `.hdr` to half float KTX2 with the KTX2 writer in `format/src/ktx2.rs`. The output is byte-identical to `assetconv`.

Image decode: done, not used yet. `yeschef/src/image.rs` decodes PNG (`png` crate) and JPEG (`zune-jpeg`) to RGBA8. On the 5 PNG files and 70 JPEG files in the game's glTF files and textures, PNG output is identical to libspng's rules and JPEG output is within 4 of 255 of libjpeg-turbo (Pillow), 0.008 on average. The converters that need it come next.

Mip filter: done, not used yet. `yeschef/src/mip.rs` ports the Mitchell halving filter and the alpha coverage helpers, with bands on the rayon pool. Its output is byte-identical to `assetconv`'s `halveMip` on 9 inputs in both color spaces (real textures up to 2048x2048 and odd-sized images with alpha).

Sprite atlas: done. `yeschef/src/atlas.rs` blits the sprites with a one texel edge gutter and writes the rect table, using `pack.rs` (the `rectangle-pack` crate plus a search for the smallest square bin). On the game's 14 sprites the atlas is byte-identical to `assetconv`'s (264x264). Other sprite sets may pack differently, because the engine reads the positions from the table.

Textures: done. `yeschef/src/texture.rs` compresses an image to BC7 (the `ctt-bc7f` crate, preset `DefaultPartiallyAnalytical`) or BC5 for normal maps (a port of the BC4 code), with a full mip chain through `mip.rs`, and bakes the manifest's `textures`. The glTF converter will use the normal and linear kinds. Against `assetconv`'s bc7enc, on the six largest Sponza and helmet textures: 39.88 dB against 39.35 dB mean PSNR, and 1.2 s against 5.5 s. BC7F is not bit-identical to bc7enc, so the files differ.

Fonts: done. `yeschef/src/font.rs` shapes with `harfrust` (a HarfBuzz port) and draws with `skrifa`'s light autohinter and `zeno` coverage, three times as wide and filtered into LCD subpixels, four phases per glyph. It checks 256 random strings per size against the shaper, as `assetconv` does. `darken.rs` adds the stem darkening FreeType applies at small sizes. Against `assetconv` on Inter at the 11 game sizes: the slot tables, all 1064 kerning pairs per size, every advance, line height and ascender are identical. Glyph bitmaps differ a little: 86% have the same size and bearings, the rest are within one pixel, and coverage is within 3% at 12 to 27 px and the same above that. The darkening amount is a measured fit, not FreeType's own number (see `darken.rs`). The atlas is 1443x1443 against 1398x1401, because `pack.rs` uses a square bin. The bake takes 2.2 s. The table layout is in `format/src/font.rs` and reaches C++ through the generated header, like the model and shader pack layouts; the baked bytes did not change when it moved. Menu screenshots (display, keyboard, audio at 1600x900) differ from `assetconv`'s by 0.02 to 0.06 of 255 on average.

glTF import: done, not used yet. `yeschef/src/import.rs` reads `.gltf` and `.glb` with the `gltf` crate into float vertices, materials and decoded images, with node transforms baked in and images decoded side by side. It reads quantized attributes itself, because the crate reads only `f32`, and it refuses a required extension that changes the data (`EXT_meshopt_compression`, Draco). On the five game models the mesh count, image count and texture slot count equal the `assetconv` bakes (Sponza: 103 meshes, 69 images, 307 slots), and Sponza loads in 0.17 s. Differences: a primitive with no normals gets flat normals on unwelded vertices, and a material with no `pbrMetallicRoughness` object takes the glTF defaults. Like `assetconv`, it does not flip the winding of a mirrored node (it warns), and it reads `TEXCOORD_0` for every texture. The tangent stage, the mesh optimizer and the model writer come next.

Models: done. `yeschef/src/model.rs` bakes a glTF or GLB: tangents (`meshopt`, when the file has none), weld, vertex cache, overdraw and fetch order, the cluster LOD hierarchy (`meshopt`'s `clusterlod`, 128 triangles per cluster, open mesh borders locked), 10:10:10:2 packed normals and tangents, and the textures through `texture.rs`. One mesh per task on the pool. The container writer is in `format/src/model.rs`. On the five game models, Sponza, both lodtest meshes and the floor have vertex, index and cluster blobs byte-identical to `assetconv`'s in every mesh, and the same file size. The helmet has the same vertices to within 2e-6, but its rotated node rounds differently in `glam` than in `glm`, so the optimizer orders them differently (278 clusters against 275, same LOD0). Baking the five models takes 3.0 s against 8.3 to 10 s for `assetconv`. The output is the same on every run.

Not done: `--verify` (`yeschef/src/convert.rs`). It fails with "not implemented yet". The orphan sweep runs only after every bake succeeds.

Exit codes: 0 success, 1 failure, 2 usage or manifest error.

CMake builds it with `-DENABLE_YESCHEF=ON`. CI runs `cargo fmt --check`, `cargo clippy -- -D warnings` and `cargo test`.

### Building with Slang

yeschef links Slang through the `shader-slang` crate, so a build needs the Slang SDK and libclang (bindgen):

- Set `SLANG_DIR` to a Slang release folder that holds `include/` and `lib/`, or set `VULKAN_SDK`.
- Put the Slang `bin` folder (Windows) or `lib` folder (Linux) on `PATH` or `LD_LIBRARY_PATH` to run it.
- Use the Slang version in `vcpkg.json`. A different version prints the same GLSL with different float digits and `#line` numbers.
- Set `LIBCLANG_PATH` if bindgen cannot find libclang.
- Building also needs a C++ compiler, for the BC7F encoder (`ctt-bc7f`). With clang on Windows, run cargo from `tools/yeschef` so `.cargo/config.toml` applies: it asks for the dynamic C runtime, which Rust's MSVC target links.
