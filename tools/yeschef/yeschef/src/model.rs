//! Bakes a glTF or GLB to a `.spnga` model: tangents, vertex order, the
//! cluster LOD hierarchy, packed vertices and BC7 or BC5 textures.
//!
//! The steps follow `assetconv`, one mesh per task on the pool:
//! 1. Generate tangents when the file has none.
//! 2. Weld, then order vertices and triangles for the cache, the overdraw and
//!    the fetch.
//! 3. Build the cluster hierarchy (`clusterlod`) for opaque meshes. A cutout or
//!    blended surface changes shape through its alpha when simplified, so it
//!    keeps LOD0 only.
//! 4. Pack the normal and tangent into 10:10:10:2 integers.

use crate::import::{self, AlphaMode, Mesh, Model, Vertex};
use crate::texture::{self, TextureKind};
use anyhow::{Context, Result, bail};
use meshopt::{
    ClodBounds, ClodMesh, SimplifyVertex, TangentOptions, VertexDataAdapter, clod_build,
    clod_default_config, generate_position_remap, generate_tangents, generate_vertex_remap,
    optimize_overdraw_in_place, optimize_vertex_cache_in_place, optimize_vertex_fetch_in_place,
    remap_index_buffer, remap_vertex_buffer, typed_to_bytes,
};
use rayon::prelude::*;
use std::collections::HashMap;
use std::path::Path;
use yeschef_format::model::{self as container, Mesh as FileMesh};
use yeschef_format::{ClusterBounds, MeshCluster, MeshEntry, PackedVertex};

/// Triangles per cluster: the `clusterlod` default for rasterization.
const CLUSTER_TRIANGLES: usize = 128;

/// The normals steer the simplifier, with the weight the meshoptimizer docs
/// suggest for unit normals.
const NORMAL_WEIGHTS: [f32; 3] = [0.5; 3];

/// `clusterlod` simplifies in permissive mode, which may collapse across a
/// seam unless the attribute that splits it is protected. Protect all five
/// floats (normal and UV): UV seams and hard normal edges stay where they are.
const PROTECTED_ATTRIBUTES: u32 = 0b11111;

/// Overdraw may cost this much vertex cache efficiency.
const OVERDRAW_THRESHOLD: f32 = 1.05;

/// A mesh ready to write.
struct Baked {
    vertices: Vec<PackedVertex>,
    indices: Vec<u32>,
    /// Empty when the mesh is one LOD0 range.
    clusters: Vec<MeshCluster>,
}

/// Replaces the vertices by one per triangle corner, each with its tangent.
fn generate_corner_tangents(vertices: &mut Vec<Vertex>, indices: &mut Vec<u32>) {
    if indices.is_empty() || vertices.is_empty() {
        return;
    }
    let positions: Vec<[f32; 3]> = vertices.iter().map(|v| v.position).collect();
    let normals: Vec<[f32; 3]> = vertices.iter().map(|v| v.normal).collect();
    let uvs: Vec<[f32; 2]> = vertices.iter().map(|v| v.tex_coords).collect();
    let tangents = generate_tangents(
        Some(indices),
        &positions,
        &normals,
        &uvs,
        TangentOptions::Compatible,
    );
    *vertices = indices
        .iter()
        .zip(&tangents)
        .map(|(&index, &tangent)| Vertex {
            tangent,
            ..vertices[index as usize]
        })
        .collect();
    *indices = (0..vertices.len() as u32).collect();
}

/// Welds equal vertices and orders vertices and triangles for the GPU.
fn optimize(vertices: &mut Vec<Vertex>, indices: &mut Vec<u32>) {
    if indices.is_empty() || vertices.is_empty() {
        return;
    }
    let (count, remap) = generate_vertex_remap(vertices, Some(indices));
    *indices = remap_index_buffer(Some(indices), vertices.len(), &remap);
    *vertices = remap_vertex_buffer(vertices, count, &remap);

    optimize_vertex_cache_in_place(indices, count);
    let adapter = VertexDataAdapter::new(typed_to_bytes(vertices), size_of::<Vertex>(), 0)
        .expect("a vertex is a whole number of floats");
    optimize_overdraw_in_place(indices, &adapter, OVERDRAW_THRESHOLD);
    fetch_order(vertices, indices);
}

/// Orders the vertices by first use. The count it keeps can be lower than the
/// remapped count; the tail is unused and must not ship.
fn fetch_order(vertices: &mut Vec<Vertex>, indices: &mut [u32]) {
    let used = optimize_vertex_fetch_in_place(indices, vertices);
    vertices.truncate(used);
}

fn to_bounds(bounds: ClodBounds) -> ClusterBounds {
    ClusterBounds {
        center: bounds.center,
        radius: bounds.radius,
        error: bounds.error,
    }
}

/// Locks every vertex on an open edge: an edge that only one triangle uses.
/// Edges are counted by position, so a UV seam is not an open edge.
/// `clusterlod` locks the borders between its own groups, but not the border
/// of the mesh, and Sponza's meshes meet along those borders. A border vertex
/// that moves opens a crack to the next mesh.
fn lock_open_borders(vertices: &[Vertex], indices: &[u32]) -> Vec<SimplifyVertex> {
    let adapter = VertexDataAdapter::new(typed_to_bytes(vertices), size_of::<Vertex>(), 0)
        .expect("a vertex is a whole number of floats");
    let remap = generate_position_remap(&adapter);

    let mut edge_uses: HashMap<u64, u32> = HashMap::new();
    for triangle in indices.as_chunks::<3>().0 {
        for e in 0..3 {
            let a = remap[triangle[e] as usize];
            let b = remap[triangle[(e + 1) % 3] as usize];
            *edge_uses
                .entry(u64::from(a.min(b)) << 32 | u64::from(a.max(b)))
                .or_default() += 1;
        }
    }

    let mut locks = vec![SimplifyVertex::None; vertices.len()];
    for (edge, uses) in edge_uses {
        if uses == 1 {
            locks[(edge >> 32) as usize] = SimplifyVertex::Lock;
            locks[(edge & 0xFFFF_FFFF) as usize] = SimplifyVertex::Lock;
        }
    }
    // `remap` points each vertex at the first vertex with its position, so
    // copy the lock to every other vertex at that position.
    (0..locks.len()).map(|v| locks[remap[v] as usize]).collect()
}

/// The cluster hierarchy over `indices`, which it rewrites in cluster order.
fn build_clusters(vertices: &mut Vec<Vertex>, indices: &mut Vec<u32>) -> Vec<MeshCluster> {
    // normal xyz, then uv
    let attributes: Vec<f32> = vertices
        .iter()
        .flat_map(|v| {
            let [x, y, z] = v.normal;
            [x, y, z, v.tex_coords[0], v.tex_coords[1]]
        })
        .collect();
    let locks = lock_open_borders(vertices, indices);
    let adapter = VertexDataAdapter::new(typed_to_bytes(vertices), size_of::<Vertex>(), 0)
        .expect("a vertex is a whole number of floats");
    let mesh = ClodMesh {
        vertex_attributes: &attributes,
        vertex_attributes_stride: 5 * size_of::<f32>(),
        vertex_lock: Some(&locks),
        attribute_weights: &NORMAL_WEIGHTS,
        attribute_protect_mask: PROTECTED_ATTRIBUTES,
        ..ClodMesh::new(indices, &adapter)
    };

    let mut out_indices: Vec<u32> = Vec::with_capacity(indices.len());
    let mut clusters = Vec::new();
    // `ClodGroup::simplified` of each group, by the id the callback returns.
    let mut groups: Vec<ClodBounds> = Vec::new();
    clod_build(
        clod_default_config(CLUSTER_TRIANGLES),
        &mesh,
        |group, group_clusters| {
            for cluster in group_clusters {
                clusters.push(MeshCluster {
                    first_index: out_indices.len() as u32,
                    index_count: cluster.indices.len() as u32,
                    depth: group.depth as u32,
                    // A simplified cluster has the error of the group it was
                    // made from. LOD0 has none.
                    bounds: if cluster.refined < 0 {
                        ClusterBounds::default()
                    } else {
                        to_bounds(groups[cluster.refined as usize])
                    },
                    parent: to_bounds(group.simplified),
                });
                out_indices.extend_from_slice(cluster.indices);
            }
            groups.push(group.simplified);
            (groups.len() - 1) as i32
        },
    );
    *indices = out_indices;

    // The triangles are in a new order, so the fetch order no longer holds.
    // Only index values change, not their positions, so the cluster ranges
    // stay valid.
    fetch_order(vertices, indices);
    clusters
}

/// `meshopt_quantizeSnorm`: -1..1 to an `n` bit signed integer.
fn quantize_snorm(v: f32, n: u32) -> i32 {
    let scale = ((1 << (n - 1)) - 1) as f32;
    let round = if v >= 0.0 { 0.5 } else { -0.5 };
    (v.clamp(-1.0, 1.0) * scale + round) as i32
}

/// x, y, z in 10 bits each and w in 2, in `GL_INT_2_10_10_10_REV` bit order.
fn pack_snorm_1010102(v: [f32; 4]) -> u32 {
    let bits = |f: f32, n: u32| (quantize_snorm(f, n) as u32) & ((1 << n) - 1);
    bits(v[0], 10) | bits(v[1], 10) << 10 | bits(v[2], 10) << 20 | bits(v[3], 2) << 30
}

fn prepare(mesh: &Mesh) -> Baked {
    let mut vertices = mesh.vertices.clone();
    let mut indices = mesh.indices.clone();
    if mesh.needs_tangents {
        generate_corner_tangents(&mut vertices, &mut indices);
    }
    optimize(&mut vertices, &mut indices);

    // A cutout or blended surface changes shape through its texture alpha
    // when simplified, so those meshes keep LOD0 only.
    let clusters = if mesh.alpha_mode == AlphaMode::Opaque && !indices.is_empty() {
        build_clusters(&mut vertices, &mut indices)
    } else {
        Vec::new()
    };

    let vertices = vertices
        .iter()
        .map(|v| PackedVertex {
            position: v.position,
            tex_coords: v.tex_coords,
            normal: pack_snorm_1010102([v.normal[0], v.normal[1], v.normal[2], 0.0]),
            tangent: pack_snorm_1010102(v.tangent),
        })
        .collect();
    Baked {
        vertices,
        indices,
        clusters,
    }
}

/// What each texture slot wants, in the order of the baked file.
const SLOT_KINDS: [TextureKind; 6] = [
    TextureKind::Color,
    TextureKind::Normal,
    TextureKind::Linear,
    TextureKind::Color,
    TextureKind::Linear,
    TextureKind::Linear,
];

struct EncodedTextures {
    files: Vec<Vec<u8>>,
    /// The file index of each slot of each mesh, or -1.
    mesh_slots: Vec<[i32; 6]>,
}

/// A KTX2 file for each image a mesh uses, in the order the meshes first use
/// them, and the file index of each mesh slot. The file does not depend on the
/// order the importer decoded in.
fn encode_textures(model: &Model) -> Result<EncodedTextures> {
    // The file holds one encoding per image, so the first slot that uses an
    // image picks its kind.
    let mut kinds: Vec<Option<TextureKind>> = vec![None; model.images.len()];
    // The alpha a texel of an image needs to pass its MASK material's test:
    // the cutoff over the base colour alpha that scales it. A shared image
    // keeps the first masked use.
    let mut cutoffs = vec![0.0f32; model.images.len()];
    let mut ids: HashMap<u32, i32> = HashMap::new();
    let mut order: Vec<u32> = Vec::new();
    let mut slots = Vec::with_capacity(model.meshes.len());

    for mesh in &model.meshes {
        if let Some(albedo) = mesh.textures.albedo
            && mesh.alpha_cutoff > 0.0
            && cutoffs[albedo as usize] == 0.0
        {
            cutoffs[albedo as usize] = mesh.alpha_cutoff / mesh.base_color_factor[3].max(1.0e-6);
        }
        let mut file_ids = [-1i32; 6];
        for (slot, (image, kind)) in mesh
            .textures
            .slots()
            .into_iter()
            .zip(SLOT_KINDS)
            .enumerate()
        {
            let Some(image) = image else { continue };
            match kinds[image as usize] {
                None => kinds[image as usize] = Some(kind),
                Some(chosen) if chosen != kind => eprintln!(
                    "yeschef: {} is used as more than one kind of texture; encoding it for its first use",
                    model.images[image as usize].name
                ),
                Some(_) => {}
            }
            file_ids[slot] = *ids.entry(image).or_insert_with(|| {
                order.push(image);
                order.len() as i32 - 1
            });
        }
        slots.push(file_ids);
    }

    let files = order
        .par_iter()
        .map(|&image| {
            let source = &model.images[image as usize];
            texture::encode(
                &source.rgba,
                kinds[image as usize].expect("a used image has a kind"),
                cutoffs[image as usize],
            )
            .with_context(|| source.name.clone())
        })
        .collect::<Result<Vec<_>>>()?;
    Ok(EncodedTextures {
        files,
        mesh_slots: slots,
    })
}

/// The size of the cluster hierarchy of a model, to see what the bake did.
fn cluster_stats(baked: &[Baked]) -> String {
    let (mut count, mut levels, mut lod0, mut coarsest, mut all) = (0, 0, 0, 0, 0);
    for mesh in baked {
        for cluster in &mesh.clusters {
            count += 1;
            levels = levels.max(cluster.depth + 1);
            lod0 += if cluster.depth == 0 {
                cluster.index_count / 3
            } else {
                0
            };
            // The cut at infinite distance: the clusters with no coarser
            // version.
            coarsest += if cluster.parent.error == f32::MAX {
                cluster.index_count / 3
            } else {
                0
            };
        }
        all += mesh.indices.len() / 3;
    }
    format!(
        "{count} clusters, {levels} levels, {lod0} triangles at LOD0, {coarsest} at the coarsest cut, {all} in all levels"
    )
}

/// The bytes of the baked model and a line on its clusters.
fn bake(model: &Model) -> Result<(Vec<u8>, String)> {
    let baked: Vec<Baked> = model.meshes.par_iter().map(prepare).collect();
    let textures = encode_textures(model)?;

    let meshes: Vec<FileMesh> = model
        .meshes
        .iter()
        .zip(&baked)
        .zip(&textures.mesh_slots)
        .map(|((mesh, baked), &texture_index)| FileMesh {
            vertices: &baked.vertices,
            indices: &baked.indices,
            clusters: &baked.clusters,
            entry: MeshEntry {
                texture_index,
                metallic_factor: mesh.metallic_factor,
                roughness_factor: mesh.roughness_factor,
                clearcoat_factor: mesh.clearcoat_factor,
                clearcoat_roughness_factor: mesh.clearcoat_roughness_factor,
                alpha_mode: mesh.alpha_mode as u32,
                alpha_cutoff: mesh.alpha_cutoff,
                double_sided: u32::from(mesh.double_sided),
                base_color_factor: mesh.base_color_factor,
                diffuse_transmission: mesh.diffuse_transmission,
                uv_transforms: mesh.uv_transforms,
                ..MeshEntry::default()
            },
        })
        .collect();
    let files: Vec<&[u8]> = textures.files.iter().map(Vec::as_slice).collect();
    let bytes = container::write(&meshes, &files).map_err(anyhow::Error::msg)?;
    Ok((bytes, cluster_stats(&baked)))
}

pub fn convert(from: &Path, to: &Path) -> Result<()> {
    let model = import::load(from)?;
    if model.meshes.is_empty() {
        bail!("no meshes imported from {}", from.display());
    }
    let (bytes, stats) = bake(&model)?;
    std::fs::write(to, &bytes).with_context(|| format!("unable to write {}", to.display()))?;
    let name = |path: &Path| path.to_string_lossy().replace('\\', "/");
    println!("{}: {stats}", name(from));
    println!(
        "{} -> {} ({} meshes, {} bytes)",
        name(from),
        name(to),
        model.meshes.len(),
        bytes.len()
    );
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::image::Rgba;
    use bytemuck::pod_read_unaligned as read;
    use yeschef_format::{AssetHeader, TextureEntry};

    /// A flat grid of `size` x `size` quads facing +z, with uv equal to xy.
    fn grid(size: u32) -> Mesh {
        let row = size + 1;
        let vertices = (0..row * row)
            .map(|i| {
                let (x, y) = ((i % row) as f32, (i / row) as f32);
                Vertex {
                    position: [x, y, (x * 0.3).sin() * 0.1],
                    normal: [0.0, 0.0, 1.0],
                    tangent: [0.0; 4],
                    tex_coords: [x / size as f32, y / size as f32],
                }
            })
            .collect();
        let mut indices = Vec::new();
        for y in 0..size {
            for x in 0..size {
                let i = y * row + x;
                indices.extend([i, i + 1, i + row + 1, i, i + row + 1, i + row]);
            }
        }
        Mesh {
            vertices,
            indices,
            needs_tangents: true,
            ..Mesh::default()
        }
    }

    fn entry(bytes: &[u8], mesh: usize) -> MeshEntry {
        let at = size_of::<AssetHeader>() + mesh * size_of::<MeshEntry>();
        read(&bytes[at..at + size_of::<MeshEntry>()])
    }

    fn clusters(bytes: &[u8], entry: &MeshEntry) -> Vec<MeshCluster> {
        (0..entry.cluster_count as usize)
            .map(|c| {
                let at = entry.cluster_offset as usize + c * size_of::<MeshCluster>();
                read(&bytes[at..at + size_of::<MeshCluster>()])
            })
            .collect()
    }

    fn unpack(v: u32) -> [f32; 3] {
        let field = |shift: u32| {
            let raw = ((v >> shift) & 0x3FF) as i32;
            let signed = (raw << 22) >> 22;
            signed as f32 / 511.0
        };
        [field(0), field(10), field(20)]
    }

    #[test]
    fn snorm_packing() {
        assert_eq!(pack_snorm_1010102([0.0; 4]), 0);
        // -1 in two bits is 0b11.
        assert_eq!(pack_snorm_1010102([0.0, 0.0, 0.0, -1.0]) >> 30, 3);
        let packed = pack_snorm_1010102([1.0, -1.0, 0.0, 0.0]);
        assert_eq!(unpack(packed), [1.0, -1.0, 0.0]);
    }

    #[test]
    fn a_grid_gets_tangents_and_a_cluster_hierarchy() {
        let model = Model {
            meshes: vec![grid(40)],
            ..Model::default()
        };
        let (bytes, _) = bake(&model).unwrap();
        let entry = entry(&bytes, 0);
        let clusters = clusters(&bytes, &entry);

        // 3200 triangles do not fit in one cluster, so the hierarchy has
        // coarser levels.
        assert!(clusters.iter().any(|c| c.depth > 0));
        let lod0: u32 = clusters
            .iter()
            .filter(|c| c.depth == 0)
            .map(|c| c.index_count)
            .sum();
        assert_eq!(lod0, 3200 * 3);
        // The LOD0 clusters come first and cover the start of the indices.
        let mut next = 0;
        for cluster in clusters.iter().take_while(|c| c.depth == 0) {
            assert_eq!(cluster.first_index, next);
            next += cluster.index_count;
        }
        assert_eq!(next, lod0);
        assert!(
            clusters
                .iter()
                .all(|c| c.first_index + c.index_count <= entry.index_count)
        );

        // The grid is welded back to its 41 x 41 vertices.
        assert!(entry.vertex_count <= 41 * 41);
        // uv equals xy, so the tangent points along +x with a positive sign.
        let at = entry.vertex_offset as usize;
        let vertex: PackedVertex = read(&bytes[at..at + size_of::<PackedVertex>()]);
        let [x, y, z] = unpack(vertex.tangent);
        assert!(x > 0.99 && y.abs() < 0.05 && z.abs() < 0.05, "{x} {y} {z}");
        assert_eq!(vertex.tangent >> 30, 1);
        assert_eq!(unpack(vertex.normal)[2], 1.0);
    }

    #[test]
    fn a_blended_mesh_keeps_lod0_only() {
        let mut mesh = grid(40);
        mesh.alpha_mode = AlphaMode::Blend;
        let model = Model {
            meshes: vec![mesh],
            ..Model::default()
        };
        let (bytes, _) = bake(&model).unwrap();
        let entry = entry(&bytes, 0);
        let clusters = clusters(&bytes, &entry);
        assert_eq!(clusters.len(), 1);
        assert_eq!(clusters[0].index_count, entry.index_count);
        assert_eq!(clusters[0].parent.error, f32::MAX);
        assert_eq!(entry.alpha_mode, 2);
    }

    #[test]
    fn textures_are_shared_and_ordered_by_first_use() {
        let image = |name: &str| import::Image {
            name: name.to_string(),
            rgba: Rgba {
                width: 8,
                height: 8,
                pixels: [200, 100, 50, 255].repeat(64),
            },
        };
        let mut first = grid(2);
        first.textures.normal = Some(1);
        first.textures.albedo = Some(0);
        let mut second = grid(2);
        second.textures.albedo = Some(0);
        second.alpha_mode = AlphaMode::Mask;
        second.alpha_cutoff = 0.5;
        let model = Model {
            images: vec![image("albedo"), image("normal")],
            meshes: vec![first, second],
        };
        let (bytes, _) = bake(&model).unwrap();

        let header: AssetHeader = read(&bytes[..size_of::<AssetHeader>()]);
        assert_eq!((header.mesh_count, header.texture_count), (2, 2));
        // The albedo is the first slot, so it is texture 0 for both meshes.
        assert_eq!(entry(&bytes, 0).texture_index, [0, 1, -1, -1, -1, -1]);
        assert_eq!(entry(&bytes, 1).texture_index, [0, -1, -1, -1, -1, -1]);
        let textures_at = size_of::<AssetHeader>() + 2 * size_of::<MeshEntry>();
        for i in 0..2 {
            let at = textures_at + i * size_of::<TextureEntry>();
            let texture: TextureEntry = read(&bytes[at..at + size_of::<TextureEntry>()]);
            assert_eq!(&bytes[texture.offset as usize + 1..][..3], b"KTX");
        }
    }

    #[test]
    fn no_meshes_is_an_error() {
        let dir = tempfile::tempdir().unwrap();
        let from = dir.path().join("a.gltf");
        std::fs::write(&from, r#"{"asset":{"version":"2.0"}}"#).unwrap();
        let error = convert(&from, &dir.path().join("a.spnga")).unwrap_err();
        assert!(error.to_string().contains("no meshes"));
    }
}
