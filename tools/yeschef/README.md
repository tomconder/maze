# yeschef

Rust workspace for the asset pipeline. It will replace `tools/assetconv`. Today it holds one crate.

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
