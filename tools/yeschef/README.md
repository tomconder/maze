# yeschef

yeschef is the asset converter for the game. It replaced the C++ converter `assetconv`, and the build runs it through CMake. The workspace has two crates, `format` and `yeschef`.

## format

`format` (`yeschef-format`) defines the byte layout of the baked files, which are the `.spnga` model container, the `shaders.spnga` shader pack and the font tables. The structs are `repr(C)`, and compile-time asserts pin their sizes.

cbindgen turns the structs into `sponge/format/scene/src/assetlayout.generated.hpp`. The engine includes that header, so it uses the layout that Rust defines. The header is checked in, and the C++ build does not run Rust.

To change the layout:

1. Edit `format/src/lib.rs`, or `format/src/font.rs` for the font tables. Bump `ASSET_VERSION` or `SHADERPACK_VERSION` if the layout changes.
2. Run `UPDATE_LAYOUT_HEADER=1 cargo test` here to rewrite the header.
3. Rebuild the C++ side. The asserts in `sponge/format/scene/src/assetformat.hpp` fail if a hand-written glm twin no longer matches.

`cargo test` fails when the checked-in header differs from what cbindgen makes.

To check the structs against real baked files, bake the assets, then run this command. Without the variable, that test passes without checking anything.

```
YESCHEF_BAKED_DIR=<bake output dir> cargo test
```

The font tables (`format/src/font.rs`, stored under the `spongeFont` key of the font KTX2 file) are in the header too. The KTX2 container is a standard format. `format/src/ktx2.rs` writes and reads it, and it is not in the header.

## yeschef

yeschef takes `--manifest <manifest.yaml> <output dir>`, `--no-line-directives`, `--no-debug-assets`, `--notices <file> [--license NAME=PATH]...` and `--verify`. It also accepts `--threads <n>`, and without it the pool has one thread per hardware thread.

Exit codes are 0 for success, 1 for a failure and 2 for a usage or manifest error. A failed bake deletes its output. The sweep that deletes files no bake listed runs only after every bake succeeds.

Each converter below was checked against `assetconv`, the C++ converter that yeschef replaced. The text says how the output compared.

- Manifest, up-to-date checks, orphan deletion and notices: the notices file is byte-identical to the `assetconv` one.
- Shader pack: `yeschef/src/shaders.rs` compiles the manifest's Slang stages to GLSL 450 and writes the pack with `format/src/shaderpack.rs`. The pack is byte-identical to the `assetconv` one, with and without line directives, when both use the same Slang version.
- Environment maps: `yeschef/src/hdr.rs` bakes Radiance `.hdr` to half float KTX2. The output is byte-identical to the `assetconv` one.
- Image decode: `yeschef/src/image.rs` decodes PNG (`png` crate) and JPEG (`zune-jpeg`) to RGBA8. On the 5 PNG files and 70 JPEG files in the game's glTF files and textures, the PNG output follows libspng's rules, and the JPEG output is within 4 of 255 of libjpeg-turbo (checked with Pillow), 0.008 on average.
- Mip filter: `yeschef/src/mip.rs` ports the Mitchell halving filter and the alpha coverage helpers, with bands on the rayon pool. The output is byte-identical to `halveMip` in `assetconv` on 9 inputs in both color spaces (real textures up to 2048x2048 and odd-sized images with alpha).
- Sprite atlas: `yeschef/src/atlas.rs` copies the sprites into one image with a one texel edge gutter and writes the rect table, using `pack.rs` (the `rectangle-pack` crate plus a search for the smallest square bin). On the game's 14 sprites the atlas is byte-identical to the `assetconv` one (264x264). Other sprite sets can pack differently, and the engine reads the positions from the table, so that is safe.
- Textures: `yeschef/src/texture.rs` compresses an image to BC7 (the `ctt-bc7f` crate, preset `DefaultPartiallyAnalytical`) or to BC5 for normal maps (a port of the BC4 code), with a full mip chain from `mip.rs`. BC7F does not give the same bits as bc7enc, so the files differ from the `assetconv` ones.
- Fonts: `yeschef/src/font.rs` shapes with `harfrust` (a HarfBuzz port) and draws with `skrifa`'s light autohinter and `zeno` coverage. It draws three times as wide and filters into LCD subpixels, with four phases per glyph. It checks 256 random strings per size against the shaper, as `assetconv` does. `darken.rs` adds the stem darkening that FreeType applies at small sizes.
- glTF import: `yeschef/src/import.rs` reads `.gltf` and `.glb` with the `gltf` crate into float vertices, materials and decoded images, with node transforms baked in and images decoded side by side. It reads quantized attributes itself, because the crate reads only `f32`, and it refuses a required extension that changes the data (`EXT_meshopt_compression`, Draco). Like `assetconv`, it does not flip the winding of a mirrored node (it warns), and it reads `TEXCOORD_0` for every texture.
- Models: `yeschef/src/model.rs` bakes a glTF or GLB. It makes tangents with `meshopt` when the file has none, then welds, orders vertices for the cache, overdraw and fetch, and builds the cluster LOD hierarchy (`meshopt`'s `clusterlod`, 128 triangles per cluster, open mesh borders locked). It then packs normals and tangents as 10:10:10:2 and bakes the textures through `texture.rs`. It runs one mesh per task on the pool. The container writer is in `format/src/model.rs`.
- `--verify <source> <baked>`: it imports the source again, runs the same steps as a bake and compares the result with the baked `.spnga` read back through `format/src/model.rs`. `read` checks magic, version, ranges and cluster blocks. The comparison covers mesh count, vertex and index counts, vertices, indices, clusters, texture presence per slot and texture size. It exits 1 on a difference.

### Comparison with assetconv before it was removed

Models: on the five game models, Sponza, both `lodtest` meshes and the floor have vertex, index and cluster blobs that are byte-identical to the `assetconv` ones in every mesh, and the same file size. The helmet has the same vertices to within 2e-6, but its rotated node rounds differently in `glam` than in `glm`, so the optimizer orders them differently (278 clusters against 275, same LOD0). Two differences come from the importer. A primitive with no normals gets flat normals on unwelded vertices, and a material with no `pbrMetallicRoughness` object takes the glTF defaults.

Fonts: against `assetconv` on Inter at the 11 game sizes, the slot tables, all 1064 kerning pairs per size, every advance, line height and ascender are identical. Glyph bitmaps differ a little. 86% have the same size and bearings, and the rest are within one pixel. Coverage is within 3% at 12 to 27 px and the same above that. The darkening amount is a measured fit and not FreeType's own number (see `darken.rs`). The atlas is 1443x1443 against 1398x1401, because `pack.rs` uses a square bin. Menu screenshots (display, keyboard, audio at 1600x900) differ from the `assetconv` ones by 0.02 to 0.06 of 255 on average.

Time: a full bake of `assets/manifest.yaml` takes 5.2 to 5.4 s against 11.7 to 12.2 s for `assetconv`, in three runs each with release builds and a clean output folder. The five models alone take 3.0 s against 8.3 to 10 s, and the output is the same on every run.

Texture quality: PSNR against the decoded source, at mip 0, has these means.

- Helmet: 50.49 dB against 51.24 dB.
- Floor: 46.26 dB against 47.15 dB.
- Sponza: 46.02 dB against 45.42 dB.

Most textures differ by about 1 dB or less in either direction, and BC5 normal maps are within 1.2 dB.

Screenshots: Sponza and the `lodtest` scene, with TAA off at 1600x900, differ from the `assetconv` bake by 0.33 and 0.22 of 255 on average. About 0.25% of the pixels differ by more than 8, mostly as speckle on foliage edges.

## Building

The game build runs yeschef. CMake builds it with cargo (`tools/yeschef/CMakeLists.txt`) and `cmake/ConvertAssets.cmake` bakes `assets/manifest.yaml` with it. It skips every output that is newer than its sources, the manifest and the yeschef binary. CI runs `cargo fmt --check`, `cargo clippy -- -D warnings` and `cargo test` in the `yeschef` job.

yeschef links Slang through the `shader-slang` crate, so a build needs Rust, libclang (for bindgen) and the Slang SDK. A CMake build takes the SDK from the vcpkg install, so `shader-slang` in `vcpkg.json` is the one place that pins the Slang version. For a plain `cargo` build in this folder:

- Set `SLANG_DIR` to a Slang release folder that holds `include/` and `lib/`, or set `VULKAN_SDK`. The vcpkg install folder for your triplet works.
- Put the Slang `bin` folder (Windows) or `lib` folder (Linux) on `PATH` or `LD_LIBRARY_PATH` to run it.
- Use the Slang version in `vcpkg.json`. A different version prints the same GLSL with different float digits and `#line` numbers.
- Set `LIBCLANG_PATH` if bindgen cannot find libclang.
- Building also needs a C++ compiler, for the BC7F encoder (`ctt-bc7f`). With clang on Windows, run cargo from `tools/yeschef` so `.cargo/config.toml` applies. It asks for the dynamic C runtime, which Rust's MSVC target links.
