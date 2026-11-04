//! Writing the model container, `.spnga`.
//!
//! Layout: the header, the mesh entries, the texture entries, then each mesh's
//! vertices, indices and clusters, then the textures. Every blob starts on a
//! 16 byte boundary, so a reader can view the buffer in place. Offsets count
//! from the start of the file. The structs are written as they sit in memory,
//! which is little endian on every platform the engine runs on.

use crate::{
    ASSET_MAGIC, ASSET_VERSION, AssetHeader, ClusterBounds, MeshCluster, MeshEntry, PackedVertex,
    TextureEntry,
};

const BLOB_ALIGNMENT: usize = 16;

/// One mesh to write. `entry` carries the material fields and the texture
/// ids; the writer fills in the offsets and counts.
pub struct Mesh<'a> {
    pub vertices: &'a [PackedVertex],
    pub indices: &'a [u32],
    /// Empty means one LOD0 cluster over all of `indices`.
    pub clusters: &'a [MeshCluster],
    pub entry: MeshEntry,
}

fn align(out: &mut Vec<u8>) {
    out.resize(out.len().next_multiple_of(BLOB_ALIGNMENT), 0);
}

/// Appends a blob on a new boundary and returns its offset.
fn blob(out: &mut Vec<u8>, bytes: &[u8]) -> Result<u32, String> {
    align(out);
    let offset = u32::try_from(out.len()).map_err(|_| "the model passes 4 GiB")?;
    out.extend_from_slice(bytes);
    Ok(offset)
}

/// The bytes of the container. `textures` are whole KTX2 files, and the
/// `texture_index` of each entry points into them. Fails when the file would
/// pass 4 GiB, which the 32 bit offsets cannot address.
pub fn write(meshes: &[Mesh], textures: &[&[u8]]) -> Result<Vec<u8>, String> {
    let header = AssetHeader {
        magic: ASSET_MAGIC.to_le_bytes(),
        version: ASSET_VERSION,
        vertex_size: size_of::<PackedVertex>() as u32,
        mesh_count: u32::try_from(meshes.len()).map_err(|_| "too many meshes")?,
        texture_count: u32::try_from(textures.len()).map_err(|_| "too many textures")?,
    };
    let tables = size_of::<AssetHeader>()
        + size_of::<MeshEntry>() * meshes.len()
        + size_of::<TextureEntry>() * textures.len();
    let mut out = vec![0u8; tables];
    out[..size_of::<AssetHeader>()].copy_from_slice(bytemuck::bytes_of(&header));

    let mut mesh_entries = Vec::with_capacity(meshes.len());
    for mesh in meshes {
        let count = |len: usize| u32::try_from(len).map_err(|_| "a mesh is too large");
        let whole = [MeshCluster {
            first_index: 0,
            index_count: count(mesh.indices.len())?,
            depth: 0,
            bounds: ClusterBounds::default(),
            parent: ClusterBounds {
                error: f32::MAX,
                ..ClusterBounds::default()
            },
        }];
        let clusters = if mesh.clusters.is_empty() {
            &whole[..]
        } else {
            mesh.clusters
        };
        mesh_entries.push(MeshEntry {
            vertex_offset: blob(&mut out, bytemuck::cast_slice(mesh.vertices))?,
            vertex_count: count(mesh.vertices.len())?,
            index_offset: blob(&mut out, bytemuck::cast_slice(mesh.indices))?,
            index_count: count(mesh.indices.len())?,
            cluster_offset: blob(&mut out, bytemuck::cast_slice(clusters))?,
            cluster_count: count(clusters.len())?,
            ..mesh.entry
        });
    }
    let mut texture_entries = Vec::with_capacity(textures.len());
    for texture in textures {
        texture_entries.push(TextureEntry {
            offset: blob(&mut out, texture)?,
            size: u32::try_from(texture.len()).map_err(|_| "a texture is too large")?,
        });
    }

    let meshes_at = size_of::<AssetHeader>();
    let textures_at = meshes_at + size_of::<MeshEntry>() * meshes.len();
    out[meshes_at..textures_at].copy_from_slice(bytemuck::cast_slice(&mesh_entries));
    out[textures_at..tables].copy_from_slice(bytemuck::cast_slice(&texture_entries));
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn vertex(x: f32) -> PackedVertex {
        PackedVertex {
            position: [x, 0.0, 0.0],
            ..PackedVertex::default()
        }
    }

    #[test]
    fn tables_blobs_and_default_cluster() {
        let vertices = [vertex(1.0), vertex(2.0), vertex(3.0)];
        let indices = [0u32, 1, 2];
        let texture = b"KTX 11";
        let mesh = Mesh {
            vertices: &vertices,
            indices: &indices,
            clusters: &[],
            entry: MeshEntry {
                texture_index: [0, -1, -1, -1, -1, -1],
                roughness_factor: 0.25,
                ..MeshEntry::default()
            },
        };
        let bytes = write(&[mesh], &[texture]).unwrap();

        let header: AssetHeader = bytemuck::pod_read_unaligned(&bytes[..size_of::<AssetHeader>()]);
        assert_eq!(header.magic, ASSET_MAGIC.to_le_bytes());
        assert_eq!((header.mesh_count, header.texture_count), (1, 1));
        let at = size_of::<AssetHeader>();
        let entry: MeshEntry =
            bytemuck::pod_read_unaligned(&bytes[at..at + size_of::<MeshEntry>()]);
        assert_eq!(entry.roughness_factor, 0.25);
        assert_eq!(entry.cluster_count, 1);
        for offset in [
            entry.vertex_offset,
            entry.index_offset,
            entry.cluster_offset,
        ] {
            assert_eq!(offset % 16, 0);
        }

        let at = entry.vertex_offset as usize + size_of::<PackedVertex>();
        let second: PackedVertex =
            bytemuck::pod_read_unaligned(&bytes[at..at + size_of::<PackedVertex>()]);
        assert_eq!(second, vertices[1]);

        let at = entry.cluster_offset as usize;
        let cluster: MeshCluster =
            bytemuck::pod_read_unaligned(&bytes[at..at + size_of::<MeshCluster>()]);
        assert_eq!(
            (cluster.first_index, cluster.index_count, cluster.depth),
            (0, 3, 0)
        );
        assert_eq!(cluster.parent.error, f32::MAX);

        let at = size_of::<AssetHeader>() + size_of::<MeshEntry>();
        let entry: TextureEntry =
            bytemuck::pod_read_unaligned(&bytes[at..at + size_of::<TextureEntry>()]);
        assert_eq!(entry.offset % 16, 0);
        assert_eq!(
            &bytes[entry.offset as usize..][..entry.size as usize],
            texture
        );
    }
}
