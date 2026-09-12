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
| `.obj` | Not yet. The engine still has the OBJ importer |
| `.png` (standalone) | Not yet |

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

Current state and known gaps:

- Images are stored **uncompressed**, in the UNORM format that matches what the
  engine uploaded before baking existed, so a bake cannot change how a model
  looks. This costs size: `cube-tex.glb` goes from 2 KB to 411 KB.
- BC7 for colour and BC5 for normals are next. BC7 needs OpenGL 4.2 or
  `ARB_texture_compression_bptc` and so will not load on macOS.
- The header carries no data format descriptor. Our reader needs only
  `vkFormat`. External KTX tools want a DFD, so add one if these files ever
  leave the build.
- Mip levels are not generated. A single-level uncompressed file is mipped by
  the driver at upload. Compressed files cannot be, so they must ship every
  level.

## Building

The converter builds with the project and is never installed with the game.

```
cmake.exe -B build --preset windows-msvc-release
cmake.exe --build build --target assetconv --config Release
```

It links `sponge::sponge` for the importers, the KTX2 codec and the format
writer. That pulls in more than a console tool needs, and it is the reason the
converter and the engine can never drift apart.
