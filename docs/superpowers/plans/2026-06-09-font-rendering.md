# Font Rendering: FreeType Coverage Atlas Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace offline MSDF atlas rendering with a runtime FreeType + stb\_rect\_pack grayscale coverage atlas, remove league-gothic, and simplify the fragment shader to direct R-channel alpha sampling.

**Architecture:** `FontAtlas` (platform-agnostic, `sponge/src/scene/`) uses FreeType to rasterize ASCII 32–126 at sizes 16 and 48, packs bitmaps with stb\_rect\_pack into a 512×512 R8 buffer, and pre-caches kerning pairs. `BitmapFont` (OpenGL, `sponge/src/platform/opengl/scene/`) owns a `FontAtlas`, uploads it as a GL R8 texture, and exposes the same `beginPass`/`render`/`endPass` API as the removed `MSDFFont`. No consumer code in `Button`, `SelectList`, or the layer files requires logic changes — only include updates and font path constants.

**Tech Stack:** FreeType 2 (vcpkg `freetype`), stb\_rect\_pack (`sponge/deps/stb/stb_rect_pack.h`), OpenGL 3.3 core, C++23

***

## File Map

**Create:**

* `sponge/src/scene/fontatlas.hpp` — `GlyphInfo` struct, `FontFaceSpec` struct, `FontAtlas` class
* `sponge/src/scene/fontatlas.cpp` — FreeType rasterize loop, stb\_rect\_pack packing, kerning pre-cache
* `sponge/src/platform/opengl/scene/bitmapfont.hpp` — `FontCreateInfo` struct, `BitmapFont` class
* `sponge/src/platform/opengl/scene/bitmapfont.cpp` — GL texture upload, glyph batching, render loop

**Delete (Task 7):**

* `sponge/src/scene/font.hpp`
* `sponge/src/scene/font.cpp`
* `sponge/src/platform/opengl/scene/msdffont.hpp`
* `sponge/src/platform/opengl/scene/msdffont.cpp`
* `assets/fonts/league-gothic.fnt`
* `assets/fonts/inter.fnt`
* `assets/fonts/inter-bold.fnt`
* `assets/fonts/fonts.png`

**Modify:**

* `vcpkg.json` — add `freetype` dependency
* `sponge/CMakeLists.txt` — `find_package(Freetype)` + `target_link_libraries`
* `assets/shaders/glsl/text.frag.glsl` — drop MSDF median, direct `.r` alpha
* `sponge/src/platform/opengl/renderer/assetmanager.hpp` — swap `MSDFFont` → `BitmapFont`
* `sponge/src/platform/opengl/renderer/assetmanager.cpp` — swap `MSDFFont` → `BitmapFont`
* `game/src/layer/introlayer.cpp` — update `fontName`/`fontPath` constants
* `game/src/layer/optionlayer.cpp` — update `fontName`/`fontPath` constants
* `game/src/layer/exitlayer.cpp` — update `fontName`/`fontPath` constants

**Add asset:**

* `assets/fonts/inter.ttf` — Inter Regular TTF (download from https://github.com/rsms/inter/releases, extract `Inter-Regular.ttf`, rename to `inter.ttf`)

***

## Task 1: Add FreeType dependency

**Files:**

* Modify: `vcpkg.json`

* Modify: `sponge/CMakeLists.txt`

* \[ ] **Step 1: Add freetype to vcpkg.json**

  In `vcpkg.json`, add this entry to the `dependencies` array (keep alphabetical order, after `fmt`):

  ```json
  {
    "name": "freetype",
    "version>=": "2.13.3"
  },
  ```

* \[ ] **Step 2: Add find\_package and link in sponge/CMakeLists.txt**

  After the existing `find_package(glfw3 ...)` block, add:

  ```cmake
  find_package(Freetype REQUIRED)
  ```

  After `target_link_libraries(sponge PRIVATE OpenGL::GL)`, add:

  ```cmake
  target_link_libraries(sponge PRIVATE Freetype::Freetype)
  ```

* \[ ] **Step 3: Install the package**

  Run vcpkg to install the new dependency:

  ```powershell
  vcpkg install
  ```

  Expected: FreeType builds and installs without error.

* \[ ] **Step 4: Commit**

  ```powershell
  git add vcpkg.json sponge/CMakeLists.txt
  git commit -m "build: add freetype dependency"
  ```

***

## Task 2: Add Inter font asset

**Files:**

* Add: `assets/fonts/inter.ttf`

* \[ ] **Step 1: Download Inter**

  Download Inter Regular from https://github.com/rsms/inter/releases (latest release). Extract `Inter-Regular.ttf` from the zip.

* \[ ] **Step 2: Copy to assets**

  ```powershell
  Copy-Item "path\to\Inter-Regular.ttf" "assets\fonts\inter.ttf"
  ```

* \[ ] **Step 3: Verify the file exists**

  ```powershell
  Test-Path "assets\fonts\inter.ttf"
  ```

  Expected: `True`

* \[ ] **Step 4: Commit**

  ```powershell
  git add assets/fonts/inter.ttf
  git commit -m "assets: add Inter Regular TTF"
  ```

***

## Task 3: Implement FontAtlas

**Files:**

* Create: `sponge/src/scene/fontatlas.hpp`

* Create: `sponge/src/scene/fontatlas.cpp`

* \[ ] **Step 1: Create fontatlas.hpp**

  ```cpp
  #pragma once

  #include <cstdint>
  #include <string>
  #include <unordered_map>
  #include <vector>

  namespace sponge::scene {

  struct GlyphInfo {
      float u, v;      // top-left UV in atlas (0..1), row-0-at-bottom GL convention
      float uvW, uvH;  // UV extent
      int   bearingX;  // horizontal bearing in pixels (pen to left edge of bitmap)
      int   bearingY;  // vertical bearing in pixels (baseline to top of bitmap)
      int   width;     // bitmap width in pixels
      int   height;    // bitmap height in pixels
      float advanceX;  // horizontal advance in pixels
  };

  struct FontFaceSpec {
      std::string path;  // absolute path to .ttf file
  };

  class FontAtlas {
  public:
      void build(const std::vector<FontFaceSpec>& faces,
                 const std::vector<uint32_t>&     sizes);

      const GlyphInfo* getGlyph(char32_t c, uint32_t size) const;
      float            getKerning(char32_t left, char32_t right,
                                  uint32_t size) const;
      float            getLineHeight(uint32_t size) const;
      float            getAscender(uint32_t size) const;

      const uint8_t* data() const {
          return atlasBuffer.data();
      }
      uint32_t atlasWidth() const {
          return atlasW;
      }
      uint32_t atlasHeight() const {
          return atlasH;
      }

  private:
      static uint64_t glyphKey(const char32_t c, const uint32_t size) {
          return (static_cast<uint64_t>(c) << 32) | size;
      }
      static uint64_t kerningKey(const char32_t left, const char32_t right,
                                 const uint32_t size) {
          return (static_cast<uint64_t>(size) << 32) |
                 (static_cast<uint64_t>(left) << 16) |
                 static_cast<uint64_t>(right);
      }

      std::unordered_map<uint64_t, GlyphInfo> glyphs;
      std::unordered_map<uint64_t, float>     kerningMap;
      std::unordered_map<uint32_t, float>     lineHeights;
      std::unordered_map<uint32_t, float>     ascenders;

      std::vector<uint8_t> atlasBuffer;
      uint32_t             atlasW = 0;
      uint32_t             atlasH = 0;
  };

  }  // namespace sponge::scene
  ```

* \[ ] **Step 2: Create fontatlas.cpp**

  ```cpp
  #include "scene/fontatlas.hpp"

  #include "logging/log.hpp"

  #include <ft2build.h>
  #include FT_FREETYPE_H

  #include <stb_rect_pack.h>

  #include <algorithm>
  #include <cassert>
  #include <vector>

  namespace sponge::scene {

  namespace {
  constexpr uint32_t kAtlasSize  = 512;
  constexpr char32_t kFirstGlyph = 32;
  constexpr char32_t kLastGlyph  = 126;
  }  // namespace

  void FontAtlas::build(const std::vector<FontFaceSpec>& faces,
                        const std::vector<uint32_t>&     sizes) {
      FT_Library library = nullptr;
      if (FT_Init_FreeType(&library) != 0) {
          SPONGE_CORE_ERROR("FreeType init failed");
          return;
      }

      struct PendingGlyph {
          char32_t             c;
          uint32_t             size;
          GlyphInfo            gi;
          std::vector<uint8_t> bitmap;
          int                  bitmapW;
          int                  bitmapH;
          int                  rectIdx;
      };

      std::vector<PendingGlyph> pending;
      std::vector<stbrp_rect>   rects;

      for (const auto& faceSpec : faces) {
          FT_Face face = nullptr;
          if (FT_New_Face(library, faceSpec.path.c_str(), 0, &face) != 0) {
              SPONGE_CORE_ERROR("Failed to load font: {}", faceSpec.path);
              continue;
          }

          for (const uint32_t size : sizes) {
              FT_Set_Pixel_Sizes(face, 0, size);

              lineHeights[size] =
                  static_cast<float>(face->size->metrics.height >> 6);
              ascenders[size] =
                  static_cast<float>(face->size->metrics.ascender >> 6);

              for (char32_t c = kFirstGlyph; c <= kLastGlyph; ++c) {
                  if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0) {
                      continue;
                  }

                  const FT_GlyphSlot g = face->glyph;
                  const int          w = static_cast<int>(g->bitmap.width);
                  const int          h = static_cast<int>(g->bitmap.rows);

                  GlyphInfo gi{};
                  gi.bearingX = g->bitmap_left;
                  gi.bearingY = g->bitmap_top;
                  gi.width    = w;
                  gi.height   = h;
                  gi.advanceX = static_cast<float>(g->advance.x >> 6);

                  if (w == 0 || h == 0) {
                      glyphs[glyphKey(c, size)] = gi;
                      continue;
                  }

                  PendingGlyph pg;
                  pg.c       = c;
                  pg.size    = size;
                  pg.gi      = gi;
                  pg.bitmapW = w;
                  pg.bitmapH = h;
                  pg.bitmap.assign(g->bitmap.buffer,
                                   g->bitmap.buffer + w * h);
                  pg.rectIdx = static_cast<int>(rects.size());
                  pending.push_back(std::move(pg));

                  stbrp_rect r{};
                  r.id = pg.rectIdx;
                  r.w  = static_cast<stbrp_coord>(w + 1);
                  r.h  = static_cast<stbrp_coord>(h + 1);
                  rects.push_back(r);
              }

              // Pre-cache kerning for all ASCII pairs at this size
              for (char32_t l = kFirstGlyph; l <= kLastGlyph; ++l) {
                  const FT_UInt li = FT_Get_Char_Index(face, l);
                  for (char32_t r = kFirstGlyph; r <= kLastGlyph; ++r) {
                      const FT_UInt ri = FT_Get_Char_Index(face, r);
                      FT_Vector     kern{};
                      if (FT_Get_Kerning(face, li, ri, FT_KERNING_DEFAULT,
                                         &kern) == 0 &&
                          kern.x != 0) {
                          kerningMap[kerningKey(l, r, size)] =
                              static_cast<float>(kern.x >> 6);
                      }
                  }
              }
          }

          FT_Done_Face(face);
      }

      FT_Done_FreeType(library);

      atlasW = kAtlasSize;
      atlasH = kAtlasSize;
      atlasBuffer.assign(atlasW * atlasH, 0);

      std::vector<stbrp_node> nodes(atlasW);
      stbrp_context           ctx{};
      stbrp_init_target(&ctx, static_cast<int>(atlasW),
                        static_cast<int>(atlasH), nodes.data(),
                        static_cast<int>(nodes.size()));
      stbrp_pack_rects(&ctx, rects.data(), static_cast<int>(rects.size()));

      for (const auto& pg : pending) {
          const auto& rect = rects[pg.rectIdx];
          GlyphInfo   gi   = pg.gi;

          if (!rect.was_packed) {
              SPONGE_CORE_WARN("Glyph 0x{:x} size {} did not fit in atlas",
                               static_cast<uint32_t>(pg.c), pg.size);
              glyphs[glyphKey(pg.c, pg.size)] = gi;
              continue;
          }

          for (int row = 0; row < pg.bitmapH; ++row) {
              std::copy(pg.bitmap.begin() + row * pg.bitmapW,
                        pg.bitmap.begin() + (row + 1) * pg.bitmapW,
                        atlasBuffer.begin() +
                            (rect.y + row) * static_cast<int>(atlasW) +
                            rect.x);
          }

          // UV convention: matches GL texture uploaded without vertical flip.
          // Texture row 0 (top of stbrp buffer) lands at V=0 in GL (bottom),
          // matching the project's downward-Y screen space + no-flip upload.
          gi.u   = static_cast<float>(rect.x) / static_cast<float>(atlasW);
          gi.v   = static_cast<float>(rect.y) / static_cast<float>(atlasH);
          gi.uvW = static_cast<float>(pg.bitmapW) / static_cast<float>(atlasW);
          gi.uvH = static_cast<float>(pg.bitmapH) / static_cast<float>(atlasH);

          glyphs[glyphKey(pg.c, pg.size)] = gi;
      }
  }

  const GlyphInfo* FontAtlas::getGlyph(const char32_t c,
                                        const uint32_t size) const {
      const auto it = glyphs.find(glyphKey(c, size));
      return it != glyphs.end() ? &it->second : nullptr;
  }

  float FontAtlas::getKerning(const char32_t left, const char32_t right,
                               const uint32_t size) const {
      const auto it = kerningMap.find(kerningKey(left, right, size));
      return it != kerningMap.end() ? it->second : 0.0F;
  }

  float FontAtlas::getLineHeight(const uint32_t size) const {
      const auto it = lineHeights.find(size);
      return it != lineHeights.end() ? it->second : static_cast<float>(size);
  }

  float FontAtlas::getAscender(const uint32_t size) const {
      const auto it = ascenders.find(size);
      return it != ascenders.end() ? it->second : static_cast<float>(size);
  }

  }  // namespace sponge::scene
  ```

* \[ ] **Step 3: Verify it compiles in isolation**

  Configure and build the sponge target only:

  ```powershell
  cmake --build out/build/ci-windows-debug --target sponge
  ```

  Expected: no errors. If FreeType headers emit W4 warnings under MSVC, add this after `find_package(Freetype REQUIRED)` in `sponge/CMakeLists.txt`:

  ```cmake
  set_target_properties(Freetype::Freetype PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
      "$<TARGET_PROPERTY:Freetype::Freetype,INTERFACE_INCLUDE_DIRECTORIES>")
  ```

* \[ ] **Step 4: Commit**

  ```powershell
  git add sponge/src/scene/fontatlas.hpp sponge/src/scene/fontatlas.cpp
  git commit -m "feat: add FontAtlas — FreeType rasterizer with stb_rect_pack"
  ```

***

## Task 4: Implement BitmapFont

**Files:**

* Create: `sponge/src/platform/opengl/scene/bitmapfont.hpp`

* Create: `sponge/src/platform/opengl/scene/bitmapfont.cpp`

* \[ ] **Step 1: Create bitmapfont.hpp**

  ```cpp
  #pragma once

  #include "core/file.hpp"
  #include "platform/opengl/renderer/indexbuffer.hpp"
  #include "platform/opengl/renderer/shader.hpp"
  #include "platform/opengl/renderer/vertexarray.hpp"
  #include "platform/opengl/renderer/vertexbuffer.hpp"
  #include "scene/fontatlas.hpp"

  #include <glm/glm.hpp>

  #include <cstdint>
  #include <memory>
  #include <string>
  #include <string_view>

  namespace sponge::platform::opengl::scene {

  struct FontCreateInfo {
      std::string name;
      std::string path;
      std::string assetsFolder = core::File::getResourceDir();
  };

  class BitmapFont {
  public:
      explicit BitmapFont(const FontCreateInfo& createInfo);
      ~BitmapFont();

      uint32_t getLength(std::string_view text, uint32_t size) const;
      uint32_t getHeight(uint32_t size) const;
      void     beginPass(uint32_t size);
      void     render(std::string_view text, const glm::vec2& position,
                      const glm::vec3& color);
      void     endPass() const;

      static std::string_view getShaderName() {
          return shaderName;
      }

  private:
      static constexpr std::string_view shaderName = "text";
      static constexpr size_t           maxLength  = 256;
      static constexpr size_t           indexCount = 6;
      static constexpr size_t           vertexCount = 8;

      uint32_t passTargetSize = 0;
      uint32_t textureId      = 0;

      sponge::scene::FontAtlas            atlas;
      std::shared_ptr<renderer::Shader>   shader;
      std::unique_ptr<renderer::VertexBuffer> vbo;
      std::unique_ptr<renderer::VertexArray>  vao;
      std::unique_ptr<renderer::IndexBuffer>  ebo;
  };

  }  // namespace sponge::platform::opengl::scene
  ```

* \[ ] **Step 2: Create bitmapfont.cpp**

  ```cpp
  #include "platform/opengl/scene/bitmapfont.hpp"

  #include "platform/opengl/renderer/assetmanager.hpp"
  #include "platform/opengl/renderer/gl.hpp"

  #include <array>
  #include <cassert>
  #include <string_view>

  namespace {
  inline constexpr std::string_view vertex = "vertex";

  std::array<uint32_t,  sponge::platform::opengl::scene::BitmapFont::maxLength *
                        sponge::platform::opengl::scene::BitmapFont::indexCount>
      batchIndices;
  std::array<glm::vec2, sponge::platform::opengl::scene::BitmapFont::maxLength *
                        sponge::platform::opengl::scene::BitmapFont::vertexCount>
      batchVertices;
  }  // namespace
  ```

  Wait — `maxLength`, `indexCount`, and `vertexCount` are `private` in the class. Instead, define them as file-local constants:

  ```cpp
  #include "platform/opengl/scene/bitmapfont.hpp"

  #include "platform/opengl/renderer/assetmanager.hpp"
  #include "platform/opengl/renderer/gl.hpp"

  #include <array>
  #include <cassert>
  #include <string_view>

  namespace {
  inline constexpr std::string_view vertex     = "vertex";
  constexpr size_t                  kMaxLength  = 256;
  constexpr size_t                  kIndexCount = 6;
  constexpr size_t                  kVertexCount = 8;

  std::array<uint32_t,  kMaxLength * kIndexCount>  batchIndices;
  std::array<glm::vec2, kMaxLength * kVertexCount> batchVertices;
  }  // namespace

  namespace sponge::platform::opengl::scene {
  using renderer::AssetManager;

  BitmapFont::BitmapFont(const FontCreateInfo& createInfo) {
      assert(!createInfo.path.empty());

      const auto shaderCreateInfo = renderer::ShaderCreateInfo{
          .name               = shaderName.data(),
          .vertexShaderPath   = "/shaders/glsl/text.vert.glsl",
          .fragmentShaderPath = "/shaders/glsl/text.frag.glsl"
      };
      shader = AssetManager::createShader(shaderCreateInfo);
      shader->bind();

      vao = renderer::VertexArray::create();
      vao->bind();

      vbo = std::make_unique<renderer::VertexBuffer>(
          nullptr, kMaxLength * kVertexCount * sizeof(glm::vec2));
      vbo->bind();

      ebo = std::make_unique<renderer::IndexBuffer>(
          nullptr, kMaxLength * kIndexCount * sizeof(uint32_t));
      ebo->bind();

      const auto program = shader->getId();
      if (const auto location =
              glGetAttribLocation(program, vertex.data());
          location != -1) {
          const auto pos = static_cast<uint32_t>(location);
          glEnableVertexAttribArray(pos);
          glVertexAttribPointer(pos, 4, GL_FLOAT, GL_FALSE,
                                4 * sizeof(GLfloat), nullptr);
      }

      vbo->unbind();
      vao->unbind();
      shader->unbind();

      // Build atlas from the TTF at the two supported pixel sizes
      const std::string ttfPath =
          createInfo.assetsFolder + createInfo.path;
      atlas.build({ { ttfPath } }, { 16, 48 });

      // Upload R8 atlas texture
      glGenTextures(1, &textureId);
      glBindTexture(GL_TEXTURE_2D, textureId);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                   static_cast<GLsizei>(atlas.atlasWidth()),
                   static_cast<GLsizei>(atlas.atlasHeight()),
                   0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glBindTexture(GL_TEXTURE_2D, 0);
  }

  BitmapFont::~BitmapFont() {
      if (textureId != 0) {
          glDeleteTextures(1, &textureId);
      }
  }

  uint32_t BitmapFont::getHeight(const uint32_t size) const {
      return static_cast<uint32_t>(atlas.getLineHeight(size));
  }

  uint32_t BitmapFont::getLength(const std::string_view text,
                                  const uint32_t         size) const {
      const auto str =
          text.length() > kMaxLength ? text.substr(0, kMaxLength) : text;

      float    x    = 0.0F;
      char32_t prev = 0;

      for (const char ch : str) {
          const auto c  = static_cast<char32_t>(ch);
          const auto* gi = atlas.getGlyph(c, size);
          if (!gi) {
              continue;
          }
          if (prev != 0) {
              x += atlas.getKerning(prev, c, size);
          }
          x += gi->advanceX;
          prev = c;
      }

      return static_cast<uint32_t>(x);
  }

  void BitmapFont::beginPass(const uint32_t size) {
      passTargetSize = size;
      vao->bind();
      shader->bind();
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureId);
  }

  void BitmapFont::render(const std::string_view text,
                           const glm::vec2&       position,
                           const glm::vec3&       color) {
      assert(passTargetSize != 0);

      const auto str =
          text.length() > kMaxLength ? text.substr(0, kMaxLength) : text;

      const float ascender = atlas.getAscender(passTargetSize);
      float       x        = position.x;
      char32_t    prev     = 0;
      uint32_t    i        = 0;

      for (const char ch : str) {
          const auto  c  = static_cast<char32_t>(ch);
          const auto* gi = atlas.getGlyph(c, passTargetSize);
          if (!gi) {
              continue;
          }

          if (prev != 0) {
              x += atlas.getKerning(prev, c, passTargetSize);
          }

          if (gi->width > 0 && gi->height > 0) {
              const float xpos = x + static_cast<float>(gi->bearingX);
              const float ypos = position.y + ascender -
                                 static_cast<float>(gi->bearingY);
              const float w = static_cast<float>(gi->width);
              const float h = static_cast<float>(gi->height);

              const float u0 = gi->u;
              const float v0 = gi->v;
              const float u1 = gi->u + gi->uvW;
              const float v1 = gi->v + gi->uvH;

              const std::array<glm::vec2, kVertexCount> vertices{ {
                  { xpos, ypos + h },     { u0, v1 },
                  { xpos, ypos },         { u0, v0 },
                  { xpos + w, ypos },     { u1, v0 },
                  { xpos + w, ypos + h }, { u1, v1 }
              } };

              std::copy(vertices.begin(), vertices.end(),
                        batchVertices.begin() +
                            static_cast<ptrdiff_t>(i * kVertexCount));

              const std::array<uint32_t, kIndexCount> indices = {
                  i * 4, (i * 4) + 2, (i * 4) + 1,
                  i * 4, (i * 4) + 3, (i * 4) + 2
              };

              std::copy(indices.begin(), indices.end(),
                        batchIndices.begin() +
                            static_cast<ptrdiff_t>(i * kIndexCount));
              ++i;
          }

          x += gi->advanceX;
          prev = c;
      }

      if (i == 0) {
          return;
      }

      shader->setFloat3("textColor", color);

      vbo->update(batchVertices.data(), i * kVertexCount * sizeof(glm::vec2));
      ebo->update(batchIndices.data(), i * kIndexCount);

      glDrawElements(GL_TRIANGLES,
                     static_cast<GLsizei>(i * kIndexCount),
                     GL_UNSIGNED_INT, nullptr);
  }

  void BitmapFont::endPass() const {
      shader->unbind();
      vao->unbind();
  }

  }  // namespace sponge::platform::opengl::scene
  ```

  > **Note on `maxLength`/`indexCount`/`vertexCount`:** The constants are defined locally in the `.cpp` anonymous namespace as `kMaxLength`, `kIndexCount`, `kVertexCount`. Remove `static constexpr size_t maxLength`, `indexCount`, `vertexCount` from the private section of `bitmapfont.hpp` — they are not needed in the header.

* \[ ] **Step 3: Build to check for compile errors**

  ```powershell
  cmake --build out/build/ci-windows-debug --target sponge
  ```

  Expected: compiles cleanly. Fix any reported errors before proceeding.

* \[ ] **Step 4: Commit**

  ```powershell
  git add sponge/src/platform/opengl/scene/bitmapfont.hpp
  git add sponge/src/platform/opengl/scene/bitmapfont.cpp
  git commit -m "feat: add BitmapFont — FreeType coverage atlas renderer"
  ```

***

## Task 5: Update the fragment shader

**Files:**

* Modify: `assets/shaders/glsl/text.frag.glsl`

* \[ ] **Step 1: Replace the shader**

  Replace the entire contents of `assets/shaders/glsl/text.frag.glsl` with:

  ```glsl
  #version 330 core

  out vec4 FragColor;

  in vec2 vTexCoord;

  uniform sampler2D text;
  uniform vec3      textColor;

  void main() {
      float coverage = texture(text, vTexCoord).r;
      FragColor = vec4(textColor, coverage);
  }
  ```

* \[ ] **Step 2: Commit**

  ```powershell
  git add assets/shaders/glsl/text.frag.glsl
  git commit -m "feat(shader): replace MSDF median with coverage alpha sampling"
  ```

***

## Task 6: Wire up BitmapFont — AssetManager and layer files

**Files:**

* Modify: `sponge/src/platform/opengl/renderer/assetmanager.hpp`

* Modify: `sponge/src/platform/opengl/renderer/assetmanager.cpp`

* Modify: `game/src/layer/introlayer.cpp`

* Modify: `game/src/layer/optionlayer.cpp`

* Modify: `game/src/layer/exitlayer.cpp`

* \[ ] **Step 1: Update assetmanager.hpp**

  Replace:

  ```cpp
  #include "platform/opengl/scene/msdffont.hpp"
  ```

  with:

  ```cpp
  #include "platform/opengl/scene/bitmapfont.hpp"
  ```

  Replace:

  ```cpp
      ASSET_MANAGER_FUNCS(Font, scene::MSDFFont, scene::FontCreateInfo,
                          fontHandler);
  ```

  with:

  ```cpp
      ASSET_MANAGER_FUNCS(Font, scene::BitmapFont, scene::FontCreateInfo,
                          fontHandler);
  ```

  Replace:

  ```cpp
      static AssetHandler<scene::MSDFFont, scene::FontCreateInfo> fontHandler;
  ```

  with:

  ```cpp
      static AssetHandler<scene::BitmapFont, scene::FontCreateInfo> fontHandler;
  ```

* \[ ] **Step 2: Update assetmanager.cpp**

  Replace:

  ```cpp
  AssetHandler<scene::MSDFFont, scene::FontCreateInfo> AssetManager::fontHandler;
  ```

  with:

  ```cpp
  AssetHandler<scene::BitmapFont, scene::FontCreateInfo> AssetManager::fontHandler;
  ```

* \[ ] **Step 3: Update introlayer.cpp**

  Replace the `fontName` and `fontPath` constants (lines 20–21):

  ```cpp
  constexpr std::string_view fontName = "league-gothic";
  constexpr std::string_view fontPath = "/fonts/league-gothic.fnt";
  ```

  with:

  ```cpp
  constexpr std::string_view fontName = "inter";
  constexpr std::string_view fontPath = "/fonts/inter.ttf";
  ```

* \[ ] **Step 4: Update optionlayer.cpp**

  Replace the `fontName` and `fontPath` constants (lines 50–51):

  ```cpp
  constexpr std::string_view fontName = "league-gothic";
  constexpr std::string_view fontPath = "/fonts/league-gothic.fnt";
  ```

  with:

  ```cpp
  constexpr std::string_view fontName = "inter";
  constexpr std::string_view fontPath = "/fonts/inter.ttf";
  ```

* \[ ] **Step 5: Update exitlayer.cpp**

  Replace the `fontName` and `fontPath` constants (lines 21–22):

  ```cpp
  constexpr std::string_view fontName = "league-gothic";
  constexpr std::string_view fontPath = "/fonts/league-gothic.fnt";
  ```

  with:

  ```cpp
  constexpr std::string_view fontName = "inter";
  constexpr std::string_view fontPath = "/fonts/inter.ttf";
  ```

* \[ ] **Step 6: Build the full project**

  ```powershell
  cmake --build out/build/ci-windows-debug
  ```

  Expected: full project compiles cleanly. Fix any errors before proceeding.

* \[ ] **Step 7: Commit**

  ```powershell
  git add sponge/src/platform/opengl/renderer/assetmanager.hpp
  git add sponge/src/platform/opengl/renderer/assetmanager.cpp
  git add game/src/layer/introlayer.cpp
  git add game/src/layer/optionlayer.cpp
  git add game/src/layer/exitlayer.cpp
  git commit -m "feat: wire BitmapFont into AssetManager and update font paths"
  ```

***

## Task 7: Remove old files

**Files:**

* Delete: `sponge/src/scene/font.hpp`, `font.cpp`

* Delete: `sponge/src/platform/opengl/scene/msdffont.hpp`, `msdffont.cpp`

* Delete: `assets/fonts/league-gothic.fnt`, `inter.fnt`, `inter-bold.fnt`, `fonts.png`

* \[ ] **Step 1: Remove source files**

  ```powershell
  git rm sponge/src/scene/font.hpp
  git rm sponge/src/scene/font.cpp
  git rm sponge/src/platform/opengl/scene/msdffont.hpp
  git rm sponge/src/platform/opengl/scene/msdffont.cpp
  ```

* \[ ] **Step 2: Remove old font assets**

  ```powershell
  git rm assets/fonts/league-gothic.fnt
  git rm assets/fonts/inter.fnt
  git rm assets/fonts/inter-bold.fnt
  git rm assets/fonts/fonts.png
  ```

* \[ ] **Step 3: Build to confirm nothing broke**

  ```powershell
  cmake --build out/build/ci-windows-debug
  ```

  Expected: clean build. The CMakeLists.txt uses `GLOB`, so removed files are automatically excluded from the next configure. If the build uses a cached configure, re-run CMake configure first:

  ```powershell
  cmake --preset ci-windows-debug
  cmake --build out/build/ci-windows-debug
  ```

* \[ ] **Step 4: Commit**

  ```powershell
  git commit -m "refactor: remove MSDF font system and offline font assets"
  ```

***

## Task 8: Verify rendering

**Goal:** Confirm text renders correctly at both sizes in the running application.

* \[ ] **Step 1: Build release config**

  ```powershell
  cmake --build out/build/ci-windows-release
  ```

* \[ ] **Step 2: Run the application**

  Launch the built executable. Navigate to the intro screen, options screen, and exit screen. Verify:

  * Button labels ("New Game", "Options", "Quit") render at size 48 with correct spacing
  * The gamepad status text renders at size 16 in the bottom-right corner
  * No visual artifacts (missing glyphs, corrupted quads, garbled text)
  * Text is AA'd (coverage-smooth edges, not aliased)

* \[ ] **Step 3: If text is flipped or mispositioned**

  If glyphs render upside-down, the UV V-axis convention is inverted. In `fontatlas.cpp`, change the V assignment from:

  ```cpp
  gi.v   = static_cast<float>(rect.y) / static_cast<float>(atlasH);
  gi.uvH = static_cast<float>(pg.bitmapH) / static_cast<float>(atlasH);
  ```

  to:

  ```cpp
  gi.v   = 1.0F - static_cast<float>(rect.y + pg.bitmapH) / static_cast<float>(atlasH);
  gi.uvH = static_cast<float>(pg.bitmapH) / static_cast<float>(atlasH);
  ```

  If glyphs are vertically offset relative to the button bounds, adjust the ascender offset in `bitmapfont.cpp`. Try `pos.y` directly (no ascender addition) and compare.

* \[ ] **Step 4: Commit any fixups**

  ```powershell
  git add -p
  git commit -m "fix(font): correct UV/bearing convention after visual verification"
  ```
