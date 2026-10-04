// Implementations of the header-only libraries installed by vcpkg.
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STBI_NO_BMP
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
