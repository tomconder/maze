# Font Rendering: FreeType Coverage Atlas

## Goal

Replace MSDF font rendering with FreeType-rasterized grayscale coverage bitmaps packed into a runtime-built atlas. Remove league-gothic; use Inter only. Drop HarfBuzz — FreeType's kerning tables are sufficient for ASCII.

Reference: https://superluminal.eu/16x-aa-font-rendering-using-coverage-masks-part-iii/

## Scope

* ASCII printable (codepoints 32–126), two sizes: 16px and 48px
* Fonts: `inter.ttf`, `inter-bold.ttf`
* Single R8 atlas texture, built once at startup
* Grayscale coverage (no subpixel/LCD rendering)
* No changes to vertex layout, index buffer strategy, or consumer API

## Components

### `FontAtlas` (`sponge/src/scene/`)

Platform-agnostic. Owns FreeType state and the packed glyph data.

```
struct GlyphInfo {
    float u, v;         // top-left UV in atlas (0..1)
    float uvW, uvH;     // UV extent
    int   bearingX;     // pixels left of origin
    int   bearingY;     // pixels above baseline
    int   width;        // bitmap width in pixels
    int   height;       // bitmap height in pixels
    float advanceX;     // horizontal advance in pixels
};
```

Key: `uint64_t = (char32_t << 32) | uint32_t(size)`. One flat `unordered_map` covers all faces and sizes.

**Build sequence:**

1. `FT_Init_FreeType`
2. For each (ttf path, size): `FT_New_Face` → `FT_Set_Pixel_Sizes(0, size)`
3. Loop ASCII 32–126: `FT_Load_Char(face, c, FT_LOAD_RENDER)` — produces `FT_RENDER_MODE_NORMAL` 8-bit bitmap
4. Stash bitmap bytes and feed rect into `stb_rect_pack`
5. Solve pack → blit each bitmap into R8 CPU buffer at packed origin
6. Pre-cache kerning: for all adjacent ASCII pairs call `FT_Get_Kerning(FT_KERNING_DEFAULT)`, store in `unordered_map<uint64_t, float>` keyed by `(left << 32) | right`
7. Upload R8 buffer as GL texture (caller's responsibility — `FontAtlas` is GL-agnostic)

Public API:

```cpp
void build(const std::vector<FontFaceSpec>& faces,
           const std::vector<uint32_t>& sizes);

const GlyphInfo* getGlyph(char32_t c, uint32_t size) const;
float getKerning(char32_t left, char32_t right) const;

const uint8_t* atlasData() const;
uint32_t atlasWidth() const;
uint32_t atlasHeight() const;
```

Atlas size: 95 glyphs × 2 sizes × 2 faces = 380 rects. A 512×512 R8 texture is sufficient; stb\_rect\_pack determines actual required size.

### `BitmapFont` (`sponge/src/platform/opengl/scene/`)

Replaces `MSDFFont`. Holds a `FontAtlas`, the GL texture, and the existing VAO/VBO/IBO.

Public API — identical to `MSDFFont`:

```cpp
explicit BitmapFont(const FontCreateInfo& createInfo);
uint32_t getLength(std::string_view text, uint32_t size);
uint32_t getHeight(uint32_t size) const;
void beginPass(uint32_t size);
void render(std::string_view text, const glm::vec2& position, const glm::vec3& color);
void endPass() const;
static std::string_view getShaderName();
```

`beginPass` binds VAO, shader, and R8 texture. No `screenPxRange` uniform.

`render` walks the string: look up `GlyphInfo` per char, apply bearing, emit quad into batch buffer, add kerning between adjacent pairs. Identical batching logic to current `MSDFFont::render`.

### Shader (`assets/shaders/glsl/text.frag.glsl`)

```glsl
#version 330 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D text;
uniform vec3 textColor;

void main() {
    float coverage = texture(text, vTexCoord).r;
    FragColor = vec4(textColor, coverage);
}
```

Remove `screenPxRange` uniform. Vertex shader unchanged.

## File Changes

### Add

* `sponge/src/scene/fontatlas.hpp`
* `sponge/src/scene/fontatlas.cpp`
* `sponge/src/platform/opengl/scene/bitmapfont.hpp`
* `sponge/src/platform/opengl/scene/bitmapfont.cpp`

### Delete

* `sponge/src/scene/font.hpp`
* `sponge/src/scene/font.cpp`
* `sponge/src/platform/opengl/scene/msdffont.hpp`
* `sponge/src/platform/opengl/scene/msdffont.cpp`
* `assets/fonts/league-gothic.fnt`
* `assets/fonts/inter.fnt`
* `assets/fonts/inter-bold.fnt`
* `assets/fonts/fonts.png`

### Modify

| File | Change |
|---|---|
| `vcpkg.json` | Add `freetype` dependency |
| `assets/shaders/glsl/text.frag.glsl` | Replace MSDF median with direct `.r` alpha |
| `sponge/src/platform/opengl/renderer/assetmanager.hpp/.cpp` | Swap `MSDFFont` → `BitmapFont` |
| `game/src/ui/button.hpp/.cpp` | Update includes |
| `game/src/ui/selectlist.hpp/.cpp` | Update includes |
| `game/src/layer/introlayer.cpp` | Update includes |
| `game/src/layer/optionlayer.cpp` | Update includes |
| `game/src/layer/exitlayer.cpp` | Update includes |
| Font path references | `.fnt` paths → `.ttf` paths |

## Dependencies

* `freetype` — added to `vcpkg.json`
* `stb_rect_pack` — already present in `sponge/deps/stb`

## Non-Goals

* HarfBuzz (FreeType kerning is sufficient for ASCII)
* Subpixel/LCD rendering
* Dynamic glyph loading at runtime
* Unicode beyond ASCII printable
