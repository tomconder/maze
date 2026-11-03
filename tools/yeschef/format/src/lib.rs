//! On-disk layout of the baked asset files: the model container (`.spnga`)
//! and the shader pack (`shaders.spnga`).
//!
//! This crate owns the layout. `cbindgen` turns it into
//! `sponge/format/scene/src/assetlayout.generated.hpp`, which the C++ readers
//! and writers include, so the converter and the engine cannot disagree.
//! Every struct is `repr(C)` and little endian. All offsets are absolute from
//! the start of the file.
//!
//! `magic` sits at offset 0 and `version` at offset 8 in both files, and they
//! stay there. Bump the version for any change to a layout, a field order, the
//! meaning of a field or the texture reference convention. The engine rejects
//! a version it does not know.

use std::mem::{offset_of, size_of};

/// Number of texture slots in a mesh entry. The order is albedo, normal,
/// occlusion, emissive, metallic-roughness, diffuse transmission.
pub const TEXTURE_SLOTS: usize = 6;

/// Model container version.
pub const ASSET_VERSION: u32 = 9;

/// Model container magic: the bytes `SPNGA\0\0\0`, read as a little endian
/// integer.
pub const ASSET_MAGIC: u64 = 0x0000_0041_474E_5053;

/// Shader pack version.
pub const SHADERPACK_VERSION: u32 = 1;

/// Shader pack magic: the bytes `SPNGSH\0\0`, read as a little endian
/// integer.
pub const SHADERPACK_MAGIC: u64 = 0x0000_4853_474E_5053;

/// A sphere and the simplification error inside it, in model units. The error
/// is how far the simplified surface can be from LOD0.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ClusterBounds {
    pub center: [f32; 3],
    pub radius: f32,
    pub error: f32,
}

/// One cluster of a mesh's cluster LOD hierarchy: a range of the mesh indices.
/// A view draws a cluster when its error test passes for `bounds` and fails
/// for `parent`. `bounds.error` is 0 for a LOD0 cluster. `parent.error` is
/// `FLT_MAX` for a cluster with no coarser version.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct MeshCluster {
    pub first_index: u32,
    pub index_count: u32,
    /// 0 is LOD0.
    pub depth: u32,
    pub bounds: ClusterBounds,
    pub parent: ClusterBounds,
}

/// What a baked model stores and what the GPU reads. `normal` and `tangent`
/// are signed normalized 10:10:10:2 integers (`GL_INT_2_10_10_10_REV`).
/// Positions and UVs stay float.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct PackedVertex {
    pub position: [f32; 3],
    pub tex_coords: [f32; 2],
    /// xyz; w unused.
    pub normal: u32,
    /// xyz; w is the bitangent sign, -1 or 1.
    pub tangent: u32,
}

/// KHR_texture_transform offset and scale. Rotation is not supported.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct UvTransform {
    pub offset: [f32; 2],
    pub scale: [f32; 2],
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct MeshUvTransforms {
    pub albedo: UvTransform,
    pub normal: UvTransform,
    pub occlusion: UvTransform,
    pub emissive: UvTransform,
    pub metallic_roughness: UvTransform,
    pub diffuse_transmission: UvTransform,
}

/// Start of a model container.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct AssetHeader {
    /// The bytes of `ASSET_MAGIC`.
    pub magic: [u8; 8],
    pub version: u32,
    /// `size_of::<PackedVertex>()` at bake time.
    pub vertex_size: u32,
    pub mesh_count: u32,
    pub texture_count: u32,
}

/// One mesh. The `mesh_count` entries follow the header. Vertex, index and
/// cluster blobs start on a 16-byte boundary.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct MeshEntry {
    pub vertex_offset: u32,
    pub vertex_count: u32,
    pub index_offset: u32,
    pub index_count: u32,
    /// `MeshCluster[cluster_count]`.
    pub cluster_offset: u32,
    /// At least 1.
    pub cluster_count: u32,
    /// Index into the texture table, or -1 when the slot is empty.
    pub texture_index: [i32; TEXTURE_SLOTS],
    pub metallic_factor: f32,
    pub roughness_factor: f32,
    pub clearcoat_factor: f32,
    pub clearcoat_roughness_factor: f32,
    /// 0 opaque, 1 mask, 2 blend.
    pub alpha_mode: u32,
    /// glTF MASK cutoff, else 0.
    pub alpha_cutoff: f32,
    /// 0 or 1.
    pub double_sided: u32,
    /// Linear RGBA.
    pub base_color_factor: [f32; 4],
    /// rgb is the color, w the factor.
    pub diffuse_transmission: [f32; 4],
    pub uv_transforms: MeshUvTransforms,
}

/// One KTX2 file stored whole. The `texture_count` entries follow the mesh
/// entries.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct TextureEntry {
    pub offset: u32,
    pub size: u32,
}

/// Start of a shader pack.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ShaderPackHeader {
    /// The bytes of `SHADERPACK_MAGIC`.
    pub magic: [u8; 8],
    pub version: u32,
    pub count: u32,
}

/// One shader. The `count` entries follow the header, sorted by name.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ShaderPackEntry {
    pub name_offset: u32,
    pub name_size: u32,
    pub source_offset: u32,
    pub source_size: u32,
}

// A size or offset change shows up here first. Change the version in the same
// edit.
const _: () = {
    assert!(ASSET_MAGIC == u64::from_le_bytes(*b"SPNGA\0\0\0"));
    assert!(SHADERPACK_MAGIC == u64::from_le_bytes(*b"SPNGSH\0\0"));
    assert!(size_of::<PackedVertex>() == 28);
    assert!(offset_of!(PackedVertex, tangent) == 24);
    assert!(size_of::<ClusterBounds>() == 20);
    assert!(size_of::<MeshCluster>() == 52);
    assert!(size_of::<UvTransform>() == 16);
    assert!(size_of::<MeshUvTransforms>() == 96);
    assert!(size_of::<AssetHeader>() == 24);
    assert!(size_of::<MeshEntry>() == 204);
    assert!(size_of::<TextureEntry>() == 8);
    assert!(size_of::<ShaderPackHeader>() == 16);
    assert!(size_of::<ShaderPackEntry>() == 16);
};
