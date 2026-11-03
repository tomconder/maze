use std::fs;
use std::mem::size_of;
use std::path::{Path, PathBuf};

use yeschef_format::*;

const KTX2_IDENTIFIER: [u8; 12] = [
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A,
];

fn header_path() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../sponge/format/scene/src/assetlayout.generated.hpp")
}

fn generate() -> String {
    let dir = env!("CARGO_MANIFEST_DIR");
    let config = cbindgen::Config::from_file(Path::new(dir).join("cbindgen.toml")).unwrap();
    let mut out = Vec::new();
    cbindgen::Builder::new()
        .with_crate(dir)
        .with_config(config)
        .generate()
        .expect("cbindgen failed")
        .write(&mut out);
    String::from_utf8(out).unwrap().replace("\r\n", "\n")
}

/// The checked-in C++ header must be what cbindgen makes from the Rust source.
/// Run with `UPDATE_LAYOUT_HEADER=1` to rewrite it.
#[test]
fn header_is_current() {
    let want = generate();
    let path = header_path();
    if std::env::var_os("UPDATE_LAYOUT_HEADER").is_some() {
        fs::write(&path, &want).unwrap();
        return;
    }
    let have = fs::read_to_string(&path)
        .unwrap_or_default()
        .replace("\r\n", "\n");
    assert!(
        have == want,
        "{} is stale. Run `UPDATE_LAYOUT_HEADER=1 cargo test` in tools/yeschef.",
        path.display()
    );
}

fn read<T: Copy>(bytes: &[u8], at: usize) -> T {
    assert!(at + size_of::<T>() <= bytes.len(), "read past the end");
    // SAFETY: the range is in bounds, T is plain old data and has no invalid
    // bit patterns, and read_unaligned needs no alignment.
    unsafe { std::ptr::read_unaligned(bytes.as_ptr().add(at).cast()) }
}

fn in_bounds(bytes: &[u8], offset: u32, size: usize) {
    assert!(offset as usize + size <= bytes.len(), "blob past the end");
}

fn check_model(bytes: &[u8]) {
    let header: AssetHeader = read(bytes, 0);
    assert_eq!(header.magic, ASSET_MAGIC.to_le_bytes());
    assert_eq!(header.version, ASSET_VERSION);
    assert_eq!(header.vertex_size as usize, size_of::<PackedVertex>());

    let meshes_at = size_of::<AssetHeader>();
    let textures_at = meshes_at + size_of::<MeshEntry>() * header.mesh_count as usize;
    let tables_end = textures_at + size_of::<TextureEntry>() * header.texture_count as usize;
    assert!(tables_end <= bytes.len());

    for i in 0..header.mesh_count as usize {
        let mesh: MeshEntry = read(bytes, meshes_at + i * size_of::<MeshEntry>());
        for offset in [mesh.vertex_offset, mesh.index_offset, mesh.cluster_offset] {
            assert_eq!(offset % 16, 0, "blob not 16-byte aligned");
            assert!(offset as usize >= tables_end);
        }
        in_bounds(
            bytes,
            mesh.vertex_offset,
            mesh.vertex_count as usize * size_of::<PackedVertex>(),
        );
        in_bounds(bytes, mesh.index_offset, mesh.index_count as usize * 4);
        assert!(mesh.cluster_count >= 1);
        in_bounds(
            bytes,
            mesh.cluster_offset,
            mesh.cluster_count as usize * size_of::<MeshCluster>(),
        );
        assert!(mesh.alpha_mode <= 2);
        assert!(mesh.double_sided <= 1);
        for id in mesh.texture_index {
            assert!(id >= -1 && id < header.texture_count as i32);
        }

        // The LOD0 clusters come first, as one block at the start.
        let (mut lod0_end, mut past_lod0) = (0u32, false);
        for c in 0..mesh.cluster_count as usize {
            let cluster: MeshCluster = read(
                bytes,
                mesh.cluster_offset as usize + c * size_of::<MeshCluster>(),
            );
            assert!(
                cluster.first_index as u64 + cluster.index_count as u64 <= mesh.index_count as u64
            );
            let lod0 = cluster.depth == 0;
            assert!(!lod0 || (!past_lod0 && cluster.first_index == lod0_end));
            lod0_end += if lod0 { cluster.index_count } else { 0 };
            past_lod0 |= !lod0;
        }
    }

    for i in 0..header.texture_count as usize {
        let texture: TextureEntry = read(bytes, textures_at + i * size_of::<TextureEntry>());
        assert_eq!(texture.offset % 16, 0);
        in_bounds(bytes, texture.offset, texture.size as usize);
        let start = texture.offset as usize;
        assert_eq!(&bytes[start..start + 12], &KTX2_IDENTIFIER);
    }
}

fn check_shader_pack(bytes: &[u8]) {
    let header: ShaderPackHeader = read(bytes, 0);
    assert_eq!(header.magic, SHADERPACK_MAGIC.to_le_bytes());
    assert_eq!(header.version, SHADERPACK_VERSION);

    let mut previous: Option<&[u8]> = None;
    for i in 0..header.count as usize {
        let entry: ShaderPackEntry = read(
            bytes,
            size_of::<ShaderPackHeader>() + i * size_of::<ShaderPackEntry>(),
        );
        in_bounds(bytes, entry.name_offset, entry.name_size as usize);
        in_bounds(bytes, entry.source_offset, entry.source_size as usize);
        let start = entry.name_offset as usize;
        let name = &bytes[start..start + entry.name_size as usize];
        assert!(previous.is_none_or(|p| p < name), "shader names not sorted");
        previous = Some(name);
    }
}

/// Reads real baked files with the Rust structs. Set `YESCHEF_BAKED_DIR` to
/// the output directory of a bake. Without it the test passes without
/// checking anything, because the baked files are not in the repository.
#[test]
fn baked_files_match_layout() {
    let Some(dir) = std::env::var_os("YESCHEF_BAKED_DIR") else {
        eprintln!("YESCHEF_BAKED_DIR is not set; skipping");
        return;
    };
    let dir = PathBuf::from(dir);

    let mut models = 0;
    for entry in fs::read_dir(dir.join("models")).unwrap() {
        let path = entry.unwrap().path();
        if path.extension().is_some_and(|e| e == "spnga") {
            check_model(&fs::read(&path).unwrap());
            models += 1;
        }
    }
    assert!(models > 0, "no models in {}", dir.display());
    check_shader_pack(&fs::read(dir.join("shaders/shaders.spnga")).unwrap());
}
