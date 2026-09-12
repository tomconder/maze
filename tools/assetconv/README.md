# assetconv

Build-time asset converter. It reads source models and writes the `.spnga`
files the engine loads.

The engine parses no third-party asset format at run time for a baked model.
All importers live here. `Model::parse()` branches on the file extension, so a
baked model and a `.glb` can coexist while the conversion grows.

## Usage

```
assetconv <source> <output.spnga>
assetconv --verify <source> <output.spnga>
assetconv --texture <output.ktx2> <input.png>
assetconv --atlas <output.ktx2> <name>=<png> ...
```

`--verify` imports the source again and compares it against the baked file:
mesh count, every vertex (position, UV, normal, tangent), every index, and
albedo dimensions. It does not compare pixel content. Use the
`debug.fixedJitter` setting and a screenshot diff for that.

CMake calls the converter once per entry in `assets/manifest.json`. Nothing is
converted by globbing.

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
`cmake/ConvertAssets.cmake` reads the manifest at configure time, so a new
entry needs a re-configure. Each generated command depends on the source, the
manifest and `sponge/src/scene/assetformat.hpp`, so editing any of those
rebakes without a clean.

Then reference the output from the game the way any other model is referenced:

```cpp
GameObject{ .name = "cube1", .path = "/models/cube.spnga" }
```

## Input formats

| Format | State |
| --- | --- |
| `.glb`, `.gltf` | Supported |
| `.png` (standalone, and into an atlas) | Supported |
| `.obj` | Not yet. The engine still has the OBJ importer |

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

`sponge/src/scene/assetformat.hpp` is the single definition of this layout and
is compiled into both the converter and the engine, so the two cannot disagree.

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
`sponge/src/scene/ktx2.{hpp,cpp}`.

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
its own with `--texture` instead, which brought the pair to 1.4 MB and 512 KB.

Known gaps:

- The BC4 encoder behind BC5 uses the eight-value ramp with a nearest-value
  index search, and nothing more. Good enough for normal maps. Swap in
  `rgbcx` if a texture ever needs better.
- The header carries no data format descriptor. Our reader needs only
  `vkFormat`. External KTX tools want a DFD, so add one if these files ever
  leave the build.
- Conversion is not fast. Sponza's 69 textures take about 130 s through the
  CPU BC7 encoder. It only runs when a source, the manifest or
  `assetformat.hpp` changes, so an incremental build does not pay it, but a
  clean build does.

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

## Building

The converter builds with the project and is never installed with the game.

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target assetconv --config Release
```

It links `sponge::sponge` for the importers, the KTX2 codec and the format
writer. That pulls in more than a console tool needs, and it is the reason the
converter and the engine can never drift apart.
