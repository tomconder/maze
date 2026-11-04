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

Not covered yet: the font tables (`Glyph` in `font.hpp`) and the KTX2 files.

## yeschef

The converter binary. It takes the command line of `assetconv`: `--manifest <manifest.yaml> <output dir>`, `--no-line-directives`, `--no-debug-assets`, `--notices <file> [--license NAME=PATH]...` and `--verify`. `--threads` is accepted and ignored for now.

Done: manifest parsing, up-to-date checks, orphan deletion and the notices file. The notices output is byte-identical to `assetconv`.

Shader pack: done. `yeschef/src/shaders.rs` compiles the manifest's Slang stages to GLSL 450 and writes the pack with `format/src/shaderpack.rs`. Against `assetconv` the pack is byte-identical, with and without line directives, when both use the same Slang version.

Environment maps: done. `yeschef/src/hdr.rs` bakes Radiance `.hdr` to half float KTX2 with the KTX2 writer in `format/src/ktx2.rs`. The output is byte-identical to `assetconv`.

Image decode: done, not used yet. `yeschef/src/image.rs` decodes PNG (`png` crate) and JPEG (`zune-jpeg`) to RGBA8. On the 5 PNG files and 70 JPEG files in the game's glTF files and textures, PNG output is identical to libspng's rules and JPEG output is within 4 of 255 of libjpeg-turbo (Pillow), 0.008 on average. The converters that need it come next.

Mip filter: done, not used yet. `yeschef/src/mip.rs` ports the Mitchell halving filter and the alpha coverage helpers, with bands on the rayon pool. Its output is byte-identical to `assetconv`'s `halveMip` on 9 inputs in both color spaces (real textures up to 2048x2048 and odd-sized images with alpha).

Not done: the other converters in `yeschef/src/convert.rs`. Each one fails with "not implemented yet" and leaves the existing output alone, and the orphan sweep runs only after every bake succeeds. Pointing yeschef at an `assetconv` output directory therefore deletes nothing. A manifest with only `licenses` works fully.

Exit codes: 0 success, 1 failure, 2 usage or manifest error.

CMake builds it with `-DENABLE_YESCHEF=ON`. CI runs `cargo fmt --check`, `cargo clippy -- -D warnings` and `cargo test`.

### Building with Slang

yeschef links Slang through the `shader-slang` crate, so a build needs the Slang SDK and libclang (bindgen):

- Set `SLANG_DIR` to a Slang release folder that holds `include/` and `lib/`, or set `VULKAN_SDK`.
- Put the Slang `bin` folder (Windows) or `lib` folder (Linux) on `PATH` or `LD_LIBRARY_PATH` to run it.
- Use the Slang version in `vcpkg.json`. A different version prints the same GLSL with different float digits and `#line` numbers.
- Set `LIBCLANG_PATH` if bindgen cannot find libclang.
