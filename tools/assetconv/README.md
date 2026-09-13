# assetconv

Build-time asset converter. It reads source models, images, fonts and shaders
and writes the `.spnga` and `.ktx2` files the engine loads.

The engine parses no third-party asset format at run time. Every importer
lives here, along with cgltf, tinyobjloader, stb_image, bc7enc,
stb_image_resize2, meshoptimizer, FreeType, HarfBuzz and the Slang compiler;
`Model::parse()` accepts `.spnga` and nothing else, `Texture` accepts `.ktx2`
and nothing else, `BitmapFont` reads a baked `.ktx2` font, and `Shader` reads
GLSL from `shaders/shaders.spnga` and nowhere else.

## Usage

```
assetconv [--threads <n>] --manifest <manifest.json> <output dir> [--no-line-directives]
          [--notices <file> [--license <name>=<path>]...]
assetconv [--threads <n>] --verify <source> <output.spnga>
```

`--threads` sets how many threads each image's BC7 blocks are split across.
The default is one per hardware thread. The output bytes do not depend on it.

`--manifest` bakes every asset the manifest lists: models, textures, fonts,
atlases and the shader pack. Sources are relative to the manifest's folder, outputs to
the output directory. `--no-line-directives` applies to the shader pack.

An output is skipped when it is newer than its sources, the manifest and
`assetconv` itself. The converter stands in for the format headers compiled
into it, so a format change rebakes everything. A failed conversion deletes
its output, so a partial file never looks current.

`--notices` joins third-party licenses into one file: the manifest's
`licenses` entries plus each `--license`. A name given more than once gets all
its files in one section. A section with one file holds that file; with
several, each file follows its file name, as `vcpkg_install_copyright` does.
The file is rewritten only when its text changes, and a missing license file
fails the run.

```json
"licenses": [
  { "name": "Inter", "files": ["fonts/LICENSE.txt"] }
]
```

CMake passes the licenses of the code compiled into the game
(`game/CMakeLists.txt`, `THIRD_PARTY_LICENSES`) and writes
`THIRD-PARTY-NOTICES.txt` next to the executable, which `install` copies.

`--verify` imports the source again and compares it against the baked file:
mesh count, every vertex (position, UV, normal, tangent), every index, and
albedo dimensions. It does not compare pixel content. With TAA on, the jitter
phase depends on the frame count, so turn TAA off before you compare
screenshots.

CMake calls `--manifest` once, on every build. With nothing changed that
costs a fraction of a second. Nothing is converted by globbing.

## Adding an asset

Add an entry to `assets/manifest.json`:

```json
{
  "models": [
    { "source": "models/gltf/cube/cube-tex.glb", "output": "models/cube.spnga" }
  ]
}
```

`source` is relative to `assets/`, `output` to the baked asset directory.
CMake never reads the manifest, so a new entry needs no re-configure. Editing
the source, the manifest or `sponge/format/scene/src/assetformat.hpp` rebakes without
a clean.

Then reference the output from the game the way any other model is referenced:

```cpp
GameObject{ .name = "cube1", .path = "/models/cube.spnga" }
```

## Input formats

| Format | State |
| --- | --- |
| `.glb`, `.gltf` | Supported |
| `.png` (standalone, and into an atlas) | Supported |
| `.obj` | Supported |
| `.slang` (into the shader pack) | Supported |
| `.ttf`, `.otf` (into a baked font) | Supported |

## Output format

One file per model. Textures are packed in as whole KTX2 blobs, so a model of
any texture count is still one file. All offsets are absolute from the start of
the file. Vertex and index blobs start on a 16-byte boundary, because `Vertex`
holds a `vec4`.

```
Header          magic "SPNGA\0\0\0", version, vertexSize, meshCount, textureCount
MeshEntry[]     vertex/index offsets and counts, 5 texture indices (-1 = empty),
                metallic and roughness factors, per-slot UV transforms
TextureEntry[]  offset, size
blobs           vertices, indices, KTX2 files
```

Texture slot order is albedo, normal, occlusion, emissive, metallic-roughness.
An image used by more than one material is stored once and referenced by index.

`sponge/format/scene/src/assetformat.hpp` is the single definition of this layout and
is compiled into both the converter and the engine, so the two cannot disagree.

Vertices are written vertex-cache, overdraw and fetch optimized. That used to
run in the engine on every load, where it had no effect at all: the `Mesh`
constructor uploads its buffers, and `optimize()` ran afterwards and only
swapped the CPU-side vectors. It is deterministic and depends on nothing but
the mesh, so the bake is where it belongs.

### Versioning

`magic` sits at offset 0 and `version` at offset 8, and they stay there. Any
future revision can identify any file before it trusts another byte.

Bump `version` for any change to the layout, field order, semantics or texture
reference convention. The engine rejects a version it does not know and logs
what it found. There is no migration path and no backward-compatible reader:
baked files never ship independently of the executable, so a rejected file is
always one a rebuild replaces.

Two asserts sit next to the version constant:

```cpp
static_assert(sizeof(Vertex) == 48);
static_assert(offsetof(Vertex, tangent) == 32);
```

They catch a vertex size change and a field reorder. Either one fires and
forces the version bump in the same edit.

## Textures

Each image becomes a KTX2 file. The reader and writer are in
`sponge/format/scene/src/ktx2.{hpp,cpp}`.

No supercompression. Basis or UASTC would need libktx to transcode at load
time, which defeats the point of baking.

Block format follows what the texture is for:

| Slot | Format | Mip filter |
| --- | --- | --- |
| albedo, emissive | BC7 UNORM | sRGB |
| normal | BC5 UNORM | linear |
| occlusion, metallic-roughness | BC7 UNORM | linear |

Filtering the mip chain in the wrong space is the classic bug here and shows
up as a brightness shift in the distance, not a crash. Colour is resampled in
sRGB; everything the shader reads as data is resampled linearly.

BC7 is UNORM rather than SRGB because that matches the `GL_RGBA8` the engine
uploaded before baking existed, so compression is the only change. Moving
albedo to sRGB is a separate, visible change.

BC7 needs OpenGL 4.2 or `ARB_texture_compression_bptc`, so baked models do
not load on macOS, which caps at 4.1. BC5 is RGTC, core since 3.0.

BC5 stores two channels. `pbr.slang` reconstructs Z as
`sqrt(1 - x² - y²)`, which is also correct for a three-channel normal map, so
baked and unbaked models go through the same path.

## Fonts

```json
"fonts": [
  { "source": "fonts/inter.ttf", "output": "fonts/inter.ktx2", "sizes": [18, 24, 32, 48] }
]
```

FreeType rasterizes each glyph at each size and at four subpixel phases as
LCD coverage, and `stb_rect_pack` packs them into a 1024x1024 `R8G8B8_UNORM`
atlas. The glyph set is printable ASCII, U+00D7 and the fixed-width (`tnum`)
forms of `0-9 : % ~ space ×`.

Shaping is baked too. HarfBuzz shapes with `liga`, `clig` and `calt` off, which
leaves a glyph per codepoint, an advance per glyph and pair kerning. The
converter records those in `font.hpp`'s tables under the key `spongeFont`,
then shapes 256 random strings per size both ways and fails the bake if
`font::shape()` differs from HarfBuzz in any glyph or advance. A font whose
shaping needs more than the previous glyph cannot bake.

Known gaps:

- A codepoint outside the set draws nothing and advances by the missing
  glyph. Add it to the set in `fontbake.cpp`.
- Fifteen `tnum` forms are mapped but have no bitmaps, as before: `( ) * + ,
  - . ; < = > [ ] { }` with tabular figures advance but draw nothing.

## Sprite atlases

UI art is packed into one sheet with `stb_rect_pack`. The rect table lives in
the container's own key/value data under the key `spongeAtlas`, one
`name x y w h` line per sprite, so an atlas is still a single file:

```
keyboard_arrows_horizontal 1 1 64 64
keyboard_arrows_vertical 67 1 64 64
```

`SpriteAtlas` reads it and hands out `Sprite`s that share one texture and draw
their own sub-rect. A name the sheet does not hold logs once and draws
nothing, rather than taking the frame down.

Atlases are uncompressed and single-level, and each sprite is surrounded by a
one-texel copy of its edge pixels. Mip levels would blend neighbouring sprites
into each other, and UI is drawn at roughly native size, so there is nothing
to gain from them. The border covers linear sampling at a sprite's edge.

Only UI art is atlased. Model textures repeat, and a repeating texture cannot
be a sub-rect of anything.

Size candidates run smallest area first and include oblong shapes, because a
square that fits by area often does not fit by packing, and the next square up
wastes three quarters of itself. The twelve 64x64 prompt icons land in 256x512.

A large image does not belong in a sheet: `blackcoffee.png` is 1024x1024, and
including it forced the whole atlas to 2048x2048 and 16.7 MB. It is baked on
on its own as a `textures` entry instead, which brought the pair to 1.4 MB and 512 KB.

Known gaps:

- The BC4 encoder behind BC5 uses the eight-value ramp with a nearest-value
  index search, and nothing more. Good enough for normal maps. Swap in
  `rgbcx` if a texture ever needs better.
- The header carries no data format descriptor. Our reader needs only
  `vkFormat`. External KTX tools want a DFD, so add one if these files ever
  leave the build.
- Conversion is not fast. Sponza's 69 textures take about 11 s through the
  CPU BC7 encoder on 8 threads. It only runs when a source, the manifest or
  the converter changes, so an incremental build does not pay it, but a
  clean build does. The manifest counts as an input to every output, so
  any manifest edit rebakes Sponza too.

## Cost and benefit

Baked files are much larger than the source, because a `.glb` stores its
images as PNG or JPEG while BC7 is a fixed byte per texel plus a third again
for the mip chain. The point is load time and GPU memory, not disk:

| | glb | spnga |
| --- | --- | --- |
| `cube-tex` | 2 KB | 138 KB |
| `DamagedHelmet` | 3.8 MB | 28.8 MB |
| `sponza` | 32.1 MB | 107.5 MB |

Time from starting a new game to the scene appearing, measured by polling
screenshots, all three models:

| | Load time |
| --- | --- |
| glTF at run time | 5497 ms |
| baked | 949 ms |

Compression is lossy, so the baked frame is not identical. Against the same
scene loaded from glTF, 44% of pixels match exactly, 48.6% differ by 1-2,
and 7 pixels of 2 073 600 differ by more than 64 — all in one cluster of
specular foliage highlights.

## Shaders

Every stage listed under `shaders.stages` in the manifest compiles into one
`shaders/shaders.spnga`. Each stage is `<source>:<entry point>`, with the source
relative to `assets/`:

```json
"shaders": {
  "output": "shaders/shaders.spnga",
  "stages": {
    "pbr.vert": "shaders/slang/pbr.slang:vertMain"
  }
}
```

The key is the name the engine asks for: `.vertexShader = "pbr.vert"`.

Slang runs in-process with the same options `slangc` was given: GLSL 450,
column-major matrices, and no `#line` directives in Release. The output is
byte-identical to what `slangc` wrote. Slang loads its GLSL module and
glslang by name at run time, so on Windows the build copies those DLLs next
to `assetconv.exe`.

The pack is one output. Slang includes are not listed in the manifest, so
every file under a stage source's folder counts as an input, and any edit
there rebuilds the pack.

```
Header   magic "SPNGSH" + two zero bytes, version, count
Entry[]  name offset and size, source offset and size
blobs    names and GLSL text, sorted by name
```

The pack has its own magic and version, separate from the model container,
so changing one layout never forces a rebake of the other.

A GL compile error has no file on disk to open any more. Release GLSL also
has no `#line` directives, so errors report lines in the generated source.
Build Debug to get Slang source lines back.

## Building

The converter builds with the project and is never installed with the game.

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target assetconv --config Release
```

It links `sponge::format`, the KTX2 codec and the format readers and writers
the engine also links, so the converter and the engine can never drift apart.
That library has no logging and no engine headers, so an engine edit does not
relink the converter or rebake the assets.
