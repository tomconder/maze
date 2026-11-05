# yeschef

yeschef is the asset converter for the game. It replaced the C++ converter `assetconv`, and the build runs it through CMake. The workspace has two crates, `format` and `yeschef`.

## format

`format` (`yeschef-format`) defines the byte layout of the baked files, which are the `.spnga` model container, the `shaders.spnga` shader pack and the font tables. The structs are `repr(C)`, and compile-time asserts pin their sizes.

cbindgen turns the structs into `sponge/src/scene/format/assetlayout.generated.hpp`. The engine includes that header, so it uses the layout that Rust defines. The header is checked in, and the C++ build does not run Rust.

To change the layout:

1. Edit `format/src/lib.rs`, or `format/src/font.rs` for the font tables. Bump `ASSET_VERSION` or `SHADERPACK_VERSION` if the layout changes.
2. Run `UPDATE_LAYOUT_HEADER=1 cargo test` here to rewrite the header.
3. Rebuild the C++ side. The asserts in `sponge/src/scene/format/assetformat.hpp` fail if a hand-written glm twin no longer matches.

`cargo test` fails when the checked-in header differs from what cbindgen makes.

The font tables (`format/src/font.rs`, stored under the `spongeFont` key of the font KTX2 file) are in the header too. The KTX2 container is a standard format. `format/src/ktx2.rs` writes and reads it, and it is not in the header.

## yeschef

yeschef takes `--manifest <manifest.yaml> <output dir>`, `--no-line-directives`, `--no-debug-assets`, `--notices <file> [--license NAME=PATH]...` and `--verify <source> <baked>`. Exit codes are 0 for success, 1 for a failure and 2 for a usage or manifest error. A failed bake deletes its output, and the sweep that deletes files no bake listed runs only after every bake succeeds. The pool has one thread per hardware thread, and `RAYON_NUM_THREADS` changes that.

Converters, in `yeschef/src`:

- `shaders.rs`: compiles the manifest's Slang stages to GLSL 450 and writes the shader pack.
- `hdr.rs`: bakes Radiance `.hdr` to half float KTX2.
- `texture.rs`: compresses an image to BC7 (`ctt-bc7f`) or, for normal maps, BC5, with a full mip chain from `mip.rs`. `image.rs` decodes PNG and JPEG.
- `atlas.rs`: packs sprites into one KTX2 image with a one texel gutter, using `pack.rs`.
- `font.rs`: shapes with `harfrust` and draws LCD coverage with `skrifa` and `zeno`, four subpixel phases per glyph. It checks random strings against the shaper. `darken.rs` adds the stem darkening FreeType applies at small sizes.
- `import.rs` and `model.rs`: read a glTF or GLB, then bake tangents, vertex order, the cluster LOD hierarchy (`meshopt`'s `clusterlod`), packed vertices and textures. One mesh runs per task on the pool.
- `--verify`: imports the source again, runs the same steps as a bake and compares the result with the baked `.spnga`. It checks mesh and vertex counts, vertices, indices, clusters and texture sizes, and exits 1 on a difference.

### Against assetconv

`assetconv`, the C++ converter that yeschef replaced, was removed after these checks. The notices file, shader pack (same Slang version), environment maps and sprite atlas were byte-identical.

- Time: a full bake takes 5.2 to 5.4 s against 11.7 to 12.2 s, three runs each, release builds, clean output.
- Models: four of the five game models had identical vertex, index and cluster blobs. The helmet has 278 clusters against 275 because `glam` and `glm` round a rotated node differently. LOD0 is the same.
- Textures: BC7F does not give the bits of bc7enc. PSNR against the source, mean at mip 0: helmet 50.49 dB against 51.24 dB, floor 46.26 against 47.15, Sponza 46.02 against 45.42.
- Fonts: slots, advances and kerning were identical. 86% of glyph bitmaps had the same size, the rest were within one pixel, and the darkening amount is a measured fit.
- Screenshots: Sponza and the `lodtest` scene, TAA off, differed by 0.33 and 0.22 of 255 on average, mostly as speckle on foliage edges.

## Building

The game build runs yeschef. CMake builds it with cargo (`tools/yeschef/CMakeLists.txt`) and `cmake/ConvertAssets.cmake` bakes `assets/manifest.yaml` with it. It skips every output that is newer than its sources, the manifest and the yeschef binary. CI runs `cargo fmt --check`, `cargo clippy -- -D warnings` and `cargo test` in the `yeschef` job.

yeschef links Slang through the `shader-slang` crate, so a build needs Rust, libclang (for bindgen) and the Slang SDK. A CMake build takes the SDK from the vcpkg install, so `shader-slang` in `vcpkg.json` is the one place that pins the Slang version. For a plain `cargo` build in this folder:

- Set `SLANG_DIR` to a Slang release folder that holds `include/` and `lib/`, or set `VULKAN_SDK`. The vcpkg install folder for your triplet works.
- Put the Slang `bin` folder (Windows) or `lib` folder (Linux) on `PATH` or `LD_LIBRARY_PATH` to run it.
- Use the Slang version in `vcpkg.json`. A different version prints the same GLSL with different float digits and `#line` numbers.
- Set `LIBCLANG_PATH` if bindgen cannot find libclang.
- Building also needs a C++ compiler, for the BC7F encoder (`ctt-bc7f`). With clang on Windows, run cargo from `tools/yeschef` so `.cargo/config.toml` applies. It asks for the dynamic C runtime, which Rust's MSVC target links.
