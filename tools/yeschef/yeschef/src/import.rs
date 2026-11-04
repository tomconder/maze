//! glTF and GLB import. It reads a model into float vertices, materials and
//! decoded images. Tangents, packing and the cluster LOD come later.
//!
//! The rules follow `assetconv`:
//! - Every node that has a mesh bakes its world transform into the vertices.
//!   Nodes are visited in index order, whether or not a scene lists them.
//! - Only triangle lists are read, and only `TEXCOORD_0`.
//! - Authored tangents are kept when the primitive has normals. Otherwise
//!   `needs_tangents` is set and the tangent stage makes them.
//! - An image that cannot be read or decoded is dropped with a message, and
//!   the meshes that used it lose that texture.
//!
//! Two differences from `assetconv`. A primitive with no normals gets flat
//! normals on unwelded vertices, so shared vertices shade right. A material
//! with no `pbrMetallicRoughness` object takes the glTF defaults (metallic 1,
//! roughness 1) because the `gltf` crate does not say whether the object was
//! there. A primitive with no material keeps the `Mesh` defaults.

use crate::image::{self, Rgba};
use anyhow::{Context, Result, bail};
use base64::Engine as _;
use glam::{Mat3, Mat4, Vec2, Vec3, Vec4};
use gltf::json::Value;
use indexmap::IndexMap;
use rayon::prelude::*;
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use yeschef_format::{MeshUvTransforms, UvTransform};

const TEXTURE_TRANSFORM: &str = "KHR_texture_transform";
const CLEARCOAT: &str = "KHR_materials_clearcoat";
const DIFFUSE_TRANSMISSION: &str = "KHR_materials_diffuse_transmission";

#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vertex {
    pub position: Vec3,
    pub normal: Vec3,
    /// xyz; w is the bitangent sign. Zero until the tangent stage runs, when
    /// the mesh has `needs_tangents`.
    pub tangent: Vec4,
    pub tex_coords: Vec2,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum AlphaMode {
    #[default]
    Opaque = 0,
    Mask = 1,
    Blend = 2,
}

/// Places in `Model::images`. A slot is `None` when the material has no such
/// texture or its image was dropped.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Textures {
    pub albedo: Option<u32>,
    pub normal: Option<u32>,
    pub occlusion: Option<u32>,
    pub emissive: Option<u32>,
    pub metallic_roughness: Option<u32>,
    pub diffuse_transmission: Option<u32>,
}

impl Textures {
    fn slots_mut(&mut self) -> [&mut Option<u32>; 6] {
        [
            &mut self.albedo,
            &mut self.normal,
            &mut self.occlusion,
            &mut self.emissive,
            &mut self.metallic_roughness,
            &mut self.diffuse_transmission,
        ]
    }
}

/// One triangle list with its material.
#[derive(Clone, Debug)]
pub struct Mesh {
    pub vertices: Vec<Vertex>,
    pub indices: Vec<u32>,
    /// The tangents are not authored and still have to be generated.
    pub needs_tangents: bool,
    pub textures: Textures,
    /// Linear RGBA.
    pub base_color_factor: [f32; 4],
    pub metallic_factor: f32,
    pub roughness_factor: f32,
    pub alpha_mode: AlphaMode,
    /// For `Mask` only, else 0.
    pub alpha_cutoff: f32,
    pub double_sided: bool,
    pub clearcoat_factor: f32,
    pub clearcoat_roughness_factor: f32,
    /// rgb is the color, w the factor.
    pub diffuse_transmission: [f32; 4],
    pub uv_transforms: MeshUvTransforms,
}

impl Default for Mesh {
    fn default() -> Self {
        Self {
            vertices: Vec::new(),
            indices: Vec::new(),
            needs_tangents: false,
            textures: Textures::default(),
            base_color_factor: [1.0; 4],
            metallic_factor: 0.0,
            roughness_factor: 0.5,
            alpha_mode: AlphaMode::Opaque,
            alpha_cutoff: 0.0,
            double_sided: false,
            clearcoat_factor: 0.0,
            clearcoat_roughness_factor: 0.0,
            diffuse_transmission: [1.0, 1.0, 1.0, 0.0],
            uv_transforms: MeshUvTransforms {
                albedo: IDENTITY,
                normal: IDENTITY,
                occlusion: IDENTITY,
                emissive: IDENTITY,
                metallic_roughness: IDENTITY,
                diffuse_transmission: IDENTITY,
            },
        }
    }
}

pub struct Image {
    /// The file and the part of it, for messages. One image has one name.
    pub name: String,
    pub rgba: Rgba,
}

#[derive(Default)]
pub struct Model {
    pub images: Vec<Image>,
    pub meshes: Vec<Mesh>,
}

const IDENTITY: UvTransform = UvTransform {
    offset: [0.0, 0.0],
    scale: [1.0, 1.0],
};

/// The files `path` reads besides itself: external buffers and images. Empty
/// for a GLB with no files, for data URIs, and when the file does not parse.
pub fn dependencies(path: &Path) -> Vec<PathBuf> {
    let Ok(bytes) = std::fs::read(path) else {
        return Vec::new();
    };
    let Ok(gltf) = gltf::Gltf::from_slice_without_validation(&bytes) else {
        return Vec::new();
    };
    let base = path.parent().unwrap_or(Path::new(""));
    let buffers = gltf.buffers().filter_map(|buffer| match buffer.source() {
        gltf::buffer::Source::Uri(uri) => Some(uri),
        gltf::buffer::Source::Bin => None,
    });
    let images = gltf.images().filter_map(|image| match image.source() {
        gltf::image::Source::Uri { uri, .. } => Some(uri),
        gltf::image::Source::View { .. } => None,
    });
    buffers
        .chain(images)
        .filter(|uri| !uri.starts_with("data:"))
        .map(|uri| base.join(percent_decode(uri)))
        .collect()
}

/// Required extensions that leave the data layout alone, or that the reader
/// below handles: quantized attributes and texture transforms.
fn supported(extension: &str) -> bool {
    matches!(extension, "KHR_mesh_quantization" | "KHR_texture_transform")
        || extension.starts_with("KHR_materials_")
}

/// Parses and validates. The `gltf` crate rejects every required extension it
/// has no type for, so this runs its validation and judges those itself.
fn parse(bytes: &[u8]) -> Result<gltf::Gltf> {
    use gltf::json::validation::{Error, Validate};
    let gltf = gltf::Gltf::from_slice_without_validation(bytes)?;
    let mut errors = Vec::new();
    gltf.as_json()
        .validate(gltf.as_json(), gltf::json::Path::new, &mut |path, error| {
            if error != Error::Unsupported {
                errors.push(format!("{}: {error}", path()));
            }
        });
    if !errors.is_empty() {
        bail!("invalid glTF: {}", errors.join("; "));
    }
    if let Some(extension) = gltf.extensions_required().find(|name| !supported(name)) {
        bail!("the file requires {extension}, which is not supported");
    }
    Ok(gltf)
}

pub fn load(path: &Path) -> Result<Model> {
    let bytes =
        std::fs::read(path).with_context(|| format!("unable to read {}", path.display()))?;
    let gltf = parse(&bytes).with_context(|| format!("unable to parse {}", path.display()))?;
    let base = path.parent().unwrap_or(Path::new(""));

    let mut blob = gltf.blob.clone();
    let buffers = gltf
        .buffers()
        .map(|buffer| {
            match buffer.source() {
                gltf::buffer::Source::Bin => blob
                    .take()
                    .context("the file refers to a binary chunk it does not have"),
                gltf::buffer::Source::Uri(uri) => read_uri(uri, base),
            }
            .and_then(|data| {
                if data.len() < buffer.length() {
                    bail!("buffer {} is shorter than it says", buffer.index());
                }
                Ok(data)
            })
        })
        .collect::<Result<Vec<_>>>()
        .with_context(|| format!("unable to load buffers of {}", path.display()))?;

    // Bake each node's world transform into its mesh's vertices so files
    // authored Z-up (or otherwise offset) render as their node hierarchy
    // intends.
    let world = world_matrices(&gltf)?;
    let primitives: Vec<_> = gltf
        .nodes()
        .filter_map(|node| node.mesh().map(|mesh| (world[node.index()], mesh)))
        .flat_map(|(world, mesh)| mesh.primitives().map(move |primitive| (world, primitive)))
        .collect();
    let parsed = primitives
        .par_iter()
        .map(|(world, primitive)| parse_primitive(primitive, world, &gltf, &buffers, path))
        .collect::<Result<Vec<_>>>()?;
    let mut meshes: Vec<Mesh> = parsed.into_iter().flatten().collect();

    let images = decode_images(&gltf, &buffers, base, path, &mut meshes);
    Ok(Model { images, meshes })
}

/// World matrix of every node, by node index.
fn world_matrices(gltf: &gltf::Document) -> Result<Vec<Mat4>> {
    let count = gltf.nodes().len();
    let mut parent = vec![None; count];
    for node in gltf.nodes() {
        for child in node.children() {
            parent[child.index()] = Some(node.index());
        }
    }
    let local: Vec<Mat4> = gltf
        .nodes()
        .map(|node| Mat4::from_cols_array_2d(&node.transform().matrix()))
        .collect();
    (0..count)
        .map(|start| {
            let mut matrix = local[start];
            let mut at = start;
            let mut steps = 0;
            while let Some(up) = parent[at] {
                matrix = local[up] * matrix;
                at = up;
                steps += 1;
                if steps > count {
                    bail!("the node hierarchy has a cycle");
                }
            }
            Ok(matrix)
        })
        .collect()
}

fn read_uri(uri: &str, base: &Path) -> Result<Vec<u8>> {
    if let Some(rest) = uri.strip_prefix("data:") {
        let (header, data) = rest.split_once(',').context("malformed data URI")?;
        if !header.ends_with(";base64") {
            bail!("a data URI is not base64");
        }
        return base64::engine::general_purpose::STANDARD
            .decode(data)
            .context("malformed base64 data URI");
    }
    let file = base.join(percent_decode(uri));
    std::fs::read(&file).with_context(|| format!("unable to read {}", file.display()))
}

/// Decodes the `%20` style escapes of a URI path.
fn percent_decode(uri: &str) -> String {
    let bytes = uri.as_bytes();
    let mut out = Vec::with_capacity(bytes.len());
    let mut i = 0;
    while i < bytes.len() {
        let hex = |at: usize| bytes.get(at).and_then(|&b| char::from(b).to_digit(16));
        match (bytes[i], hex(i + 1), hex(i + 2)) {
            (b'%', Some(high), Some(low)) => {
                out.push((high * 16 + low) as u8);
                i += 3;
            }
            (byte, ..) => {
                out.push(byte);
                i += 1;
            }
        }
    }
    String::from_utf8_lossy(&out).into_owned()
}

/// An image a mesh uses, and the texture transform of the view.
struct View {
    image: u32,
    uv: UvTransform,
}

fn view(texture: gltf::Texture, transform: Option<&Value>) -> View {
    View {
        image: texture.source().index() as u32,
        uv: uv_transform(transform),
    }
}

fn pair(value: Option<&Value>, default: [f32; 2]) -> [f32; 2] {
    let number = |i: usize| value?.get(i)?.as_f64().map(|v| v as f32);
    [
        number(0).unwrap_or(default[0]),
        number(1).unwrap_or(default[1]),
    ]
}

/// KHR_texture_transform offset and scale. Rotation is not supported.
fn uv_transform(transform: Option<&Value>) -> UvTransform {
    let Some(transform) = transform else {
        return IDENTITY;
    };
    if transform
        .get("rotation")
        .and_then(Value::as_f64)
        .is_some_and(|rotation| rotation != 0.0)
    {
        eprintln!("yeschef: KHR_texture_transform rotation is not supported; ignoring");
    }
    UvTransform {
        offset: pair(transform.get("offset"), IDENTITY.offset),
        scale: pair(transform.get("scale"), IDENTITY.scale),
    }
}

/// Reads an attribute of any component type as floats. A normalized integer
/// maps to 0..1 or -1..1, as `KHR_mesh_quantization` and the glTF accessor
/// rules say. The `gltf` crate's own attribute readers take `f32` only.
fn read_floats<const N: usize>(
    accessor: &gltf::Accessor,
    buffers: &[Vec<u8>],
) -> Result<Vec<[f32; N]>>
where
    [i8; N]: gltf::accessor::Item + Copy,
    [u8; N]: gltf::accessor::Item + Copy,
    [i16; N]: gltf::accessor::Item + Copy,
    [u16; N]: gltf::accessor::Item + Copy,
    [f32; N]: gltf::accessor::Item + Copy,
{
    use gltf::accessor::DataType;
    fn gather<T, const N: usize>(
        accessor: &gltf::Accessor,
        buffers: &[Vec<u8>],
        convert: impl Fn(T) -> f32,
    ) -> Result<Vec<[f32; N]>>
    where
        [T; N]: gltf::accessor::Item + Copy,
    {
        let iter = gltf::accessor::Iter::<[T; N]>::new(accessor.clone(), |buffer| {
            buffers.get(buffer.index()).map(Vec::as_slice)
        })
        .context("an attribute has no data")?;
        Ok(iter.map(|value| value.map(&convert)).collect())
    }

    let normalized = accessor.normalized();
    match accessor.data_type() {
        DataType::I8 => gather::<i8, N>(accessor, buffers, |v| {
            if normalized {
                (f32::from(v) / 127.0).max(-1.0)
            } else {
                f32::from(v)
            }
        }),
        DataType::U8 => gather::<u8, N>(accessor, buffers, |v| {
            if normalized {
                f32::from(v) / 255.0
            } else {
                f32::from(v)
            }
        }),
        DataType::I16 => gather::<i16, N>(accessor, buffers, |v| {
            if normalized {
                (f32::from(v) / 32767.0).max(-1.0)
            } else {
                f32::from(v)
            }
        }),
        DataType::U16 => gather::<u16, N>(accessor, buffers, |v| {
            if normalized {
                f32::from(v) / 65535.0
            } else {
                f32::from(v)
            }
        }),
        DataType::F32 => gather::<f32, N>(accessor, buffers, |v| v),
        DataType::U32 => bail!("an attribute has 32 bit integers"),
    }
}

/// Checks an attribute against the position count.
fn attribute<T>(values: Option<Vec<T>>, count: usize, what: &str) -> Result<Option<Vec<T>>> {
    match values {
        Some(values) if values.len() != count => {
            bail!("{what} has {} entries for {count} vertices", values.len())
        }
        values => Ok(values),
    }
}

fn parse_primitive(
    primitive: &gltf::Primitive,
    world: &Mat4,
    document: &gltf::Document,
    buffers: &[Vec<u8>],
    path: &Path,
) -> Result<Option<Mesh>> {
    if primitive.mode() != gltf::mesh::Mode::Triangles {
        return Ok(None);
    }
    use gltf::Semantic;
    let reader = primitive.reader(|buffer| buffers.get(buffer.index()).map(Vec::as_slice));
    let Some(positions) = primitive
        .get(&Semantic::Positions)
        .map(|accessor| read_floats::<3>(&accessor, buffers))
        .transpose()?
    else {
        return Ok(None);
    };
    let count = positions.len();

    let normals = primitive
        .get(&Semantic::Normals)
        .map(|accessor| read_floats::<3>(&accessor, buffers))
        .transpose()?;
    let tangents = primitive
        .get(&Semantic::Tangents)
        .map(|accessor| read_floats::<4>(&accessor, buffers))
        .transpose()?;
    let tex_coords = primitive
        .get(&Semantic::TexCoords(0))
        .map(|accessor| read_floats::<2>(&accessor, buffers))
        .transpose()?;
    let normals = attribute(normals, count, "NORMAL")?;
    let tangents = attribute(tangents, count, "TANGENT")?;
    let tex_coords = attribute(tex_coords, count, "TEXCOORD_0")?;

    let mut indices: Vec<u32> = match reader.read_indices() {
        Some(indices) => indices.into_u32().collect(),
        None => (0..count as u32).collect(),
    };
    indices.truncate(indices.len() / 3 * 3);
    if indices.iter().any(|&index| index as usize >= count) {
        bail!("{}: an index is past the last vertex", path.display());
    }

    if world.determinant() < 0.0 {
        eprintln!(
            "yeschef: {}: a node mirrors its mesh; the triangle winding is not flipped",
            path.display()
        );
    }
    let normal_matrix = Mat3::from_mat4(*world).inverse().transpose();
    let tangent_matrix = Mat3::from_mat4(*world);

    let mut vertices: Vec<Vertex> = (0..count)
        .map(|i| {
            let mut vertex = Vertex {
                position: world.transform_point3(Vec3::from(positions[i])),
                ..Vertex::default()
            };
            if let Some(coords) = &tex_coords {
                vertex.tex_coords = Vec2::from(coords[i]);
            }
            if let Some(normals) = &normals {
                vertex.normal = (normal_matrix * Vec3::from(normals[i])).normalize_or_zero();
            }
            if let (Some(tangents), Some(_)) = (&tangents, &normals) {
                let [x, y, z, w] = tangents[i];
                let direction = (tangent_matrix * Vec3::new(x, y, z)).normalize_or_zero();
                vertex.tangent = direction.extend(w);
            }
            vertex
        })
        .collect();

    if normals.is_none() {
        // A flat normal per corner, which needs the vertices unwelded.
        vertices = indices.iter().map(|&i| vertices[i as usize]).collect();
        indices = (0..vertices.len() as u32).collect();
        for triangle in vertices.as_chunks_mut::<3>().0 {
            let edge = |a: usize, b: usize| triangle[b].position - triangle[a].position;
            let normal = edge(0, 1).cross(edge(0, 2)).normalize_or_zero();
            for vertex in triangle {
                vertex.normal = normal;
            }
        }
    }

    // glTF ignores authored tangents when normals are missing, since they
    // were built against normals the file does not have.
    let mut mesh = Mesh {
        vertices,
        indices,
        needs_tangents: tangents.is_none() || normals.is_none(),
        ..Mesh::default()
    };
    apply_material(&mut mesh, &primitive.material(), document);
    Ok(Some(mesh))
}

/// Copies the material into the mesh. `mesh.textures` holds glTF image
/// indices until `decode_images` replaces them.
fn apply_material(mesh: &mut Mesh, material: &gltf::Material, document: &gltf::Document) {
    if material.index().is_none() {
        return;
    }
    let pbr = material.pbr_metallic_roughness();
    let texture_view = |info: Option<gltf::texture::Info>| {
        info.map(|info| view(info.texture(), info.extension_value(TEXTURE_TRANSFORM)))
    };
    let albedo = texture_view(pbr.base_color_texture());
    let metallic_roughness = texture_view(pbr.metallic_roughness_texture());
    let emissive = texture_view(material.emissive_texture());
    let normal = material
        .normal_texture()
        .map(|t| view(t.texture(), t.extension_value(TEXTURE_TRANSFORM)));
    let occlusion = material
        .occlusion_texture()
        .map(|t| view(t.texture(), t.extension_value(TEXTURE_TRANSFORM)));

    mesh.base_color_factor = pbr.base_color_factor();
    mesh.metallic_factor = pbr.metallic_factor();
    mesh.roughness_factor = pbr.roughness_factor();
    mesh.double_sided = material.double_sided();
    match material.alpha_mode() {
        gltf::material::AlphaMode::Mask => {
            mesh.alpha_mode = AlphaMode::Mask;
            mesh.alpha_cutoff = material.alpha_cutoff().unwrap_or(0.5);
        }
        gltf::material::AlphaMode::Blend => mesh.alpha_mode = AlphaMode::Blend,
        gltf::material::AlphaMode::Opaque => {}
    }

    if let Some(coat) = material.extension_value(CLEARCOAT) {
        let factor = |key| coat.get(key).and_then(Value::as_f64).unwrap_or(0.0) as f32;
        mesh.clearcoat_factor = factor("clearcoatFactor");
        mesh.clearcoat_roughness_factor = factor("clearcoatRoughnessFactor");
    }

    let mut transmission = None;
    if let Some(ext) = material.extension_value(DIFFUSE_TRANSMISSION) {
        let number = |key: &str, default: f64| {
            ext.get(key).and_then(Value::as_f64).unwrap_or(default) as f32
        };
        let color = ext.get("diffuseTransmissionColorFactor");
        let channel = |i: usize| {
            color
                .and_then(|c| c.get(i))
                .and_then(Value::as_f64)
                .unwrap_or(1.0) as f32
        };
        mesh.diffuse_transmission = [
            channel(0),
            channel(1),
            channel(2),
            number("diffuseTransmissionFactor", 0.0),
        ];
        let map = ext.get("diffuseTransmissionTexture");
        let texture = map
            .and_then(|map| map.get("index"))
            .and_then(Value::as_u64)
            .and_then(|index| document.textures().nth(index as usize));
        transmission = texture.map(|texture| {
            let transform = map
                .and_then(|map| map.get("extensions"))
                .and_then(|extensions| extensions.get(TEXTURE_TRANSFORM));
            view(texture, transform)
        });
    }

    let slots = [
        (&albedo, &mut mesh.uv_transforms.albedo),
        (&normal, &mut mesh.uv_transforms.normal),
        (&occlusion, &mut mesh.uv_transforms.occlusion),
        (&emissive, &mut mesh.uv_transforms.emissive),
        (
            &metallic_roughness,
            &mut mesh.uv_transforms.metallic_roughness,
        ),
        (&transmission, &mut mesh.uv_transforms.diffuse_transmission),
    ];
    let mut images = mesh.textures.slots_mut();
    for ((view, uv), image) in slots.into_iter().zip(&mut images) {
        if let Some(view) = view {
            **image = Some(view.image);
            *uv = view.uv;
        }
    }
}

/// Decodes the images the meshes use, side by side, and points the meshes at
/// their place in the result. An image used twice, even through two glTF
/// image entries, decodes once.
fn decode_images(
    document: &gltf::Document,
    buffers: &[Vec<u8>],
    base: &Path,
    path: &Path,
    meshes: &mut [Mesh],
) -> Vec<Image> {
    let images: Vec<gltf::Image> = document.images().collect();
    let name_of = |image: &gltf::Image| match image.source() {
        gltf::image::Source::View { view, .. } => format!(
            "{}#{}_{}_{}",
            path.display(),
            view.buffer().index(),
            view.offset(),
            view.length()
        ),
        gltf::image::Source::Uri { uri, .. } => format!("{}#{uri}", path.display()),
    };

    // One entry per distinct image, in the order the meshes first use them.
    let mut unique: IndexMap<String, usize> = IndexMap::new();
    let mut name_by_gltf_index: HashMap<u32, String> = HashMap::new();
    for mesh in meshes.iter_mut() {
        for slot in mesh.textures.slots_mut() {
            let Some(index) = *slot else { continue };
            let name = name_by_gltf_index
                .entry(index)
                .or_insert_with(|| name_of(&images[index as usize]))
                .clone();
            unique.entry(name).or_insert(index as usize);
        }
    }

    let unique: Vec<(String, usize)> = unique.into_iter().collect();
    let decoded: Vec<Option<Image>> = unique
        .par_iter()
        .map(
            |(name, index)| match load_image(&images[*index], buffers, base) {
                Ok(rgba) => Some(Image {
                    name: name.clone(),
                    rgba,
                }),
                Err(error) => {
                    eprintln!("yeschef: {name}: {error:#}");
                    None
                }
            },
        )
        .collect();

    // Where each name lands once the failures are gone.
    let kept: Vec<Image> = decoded.into_iter().flatten().collect();
    let placed: HashMap<&str, u32> = kept
        .iter()
        .enumerate()
        .map(|(place, image)| (image.name.as_str(), place as u32))
        .collect();
    for mesh in meshes.iter_mut() {
        for slot in mesh.textures.slots_mut() {
            if let Some(index) = *slot {
                *slot = placed.get(name_by_gltf_index[&index].as_str()).copied();
            }
        }
    }
    kept
}

fn load_image(image: &gltf::Image, buffers: &[Vec<u8>], base: &Path) -> Result<Rgba> {
    match image.source() {
        gltf::image::Source::View { view, .. } => {
            let buffer = buffers
                .get(view.buffer().index())
                .context("the image is in a missing buffer")?;
            let bytes = buffer
                .get(view.offset()..view.offset() + view.length())
                .context("the image is past the end of its buffer")?;
            image::decode(bytes)
        }
        gltf::image::Source::Uri { uri, .. } => image::decode(&read_uri(uri, base)?),
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use base64::engine::general_purpose::STANDARD;

    fn floats(values: &[f32]) -> Vec<u8> {
        values.iter().flat_map(|v| v.to_le_bytes()).collect()
    }

    fn data_uri(bytes: &[u8]) -> String {
        format!(
            "data:application/octet-stream;base64,{}",
            STANDARD.encode(bytes)
        )
    }

    fn png_bytes(width: u32, height: u32, rgba: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        let mut encoder = png::Encoder::new(&mut out, width, height);
        encoder.set_color(png::ColorType::Rgba);
        encoder.set_depth(png::BitDepth::Eight);
        encoder
            .write_header()
            .unwrap()
            .write_image_data(rgba)
            .unwrap();
        out
    }

    /// One triangle at (0,0,0) (1,0,0) (0,1,0).
    const TRIANGLE: [f32; 9] = [0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0];

    fn gltf_json(buffer_uri: &str, extra: &str) -> String {
        format!(
            r#"{{"asset":{{"version":"2.0"}},
            "buffers":[{{"byteLength":36,"uri":"{buffer_uri}"}}],
            "bufferViews":[{{"buffer":0,"byteLength":36}}],
            "accessors":[{{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3",
                "min":[0,0,0],"max":[1,1,0]}}],
            "meshes":[{{"primitives":[{{"attributes":{{"POSITION":0}}{extra}}}]}}],
            "nodes":[{{"mesh":0,"translation":[1,2,3],"scale":[2,2,2]}}],
            "scenes":[{{"nodes":[0]}}],"scene":0}}"#
        )
    }

    fn write(dir: &Path, name: &str, bytes: &[u8]) -> PathBuf {
        let path = dir.join(name);
        std::fs::write(&path, bytes).unwrap();
        path
    }

    #[test]
    fn node_transform_and_flat_normals() {
        let dir = tempfile::tempdir().unwrap();
        let json = gltf_json(&data_uri(&floats(&TRIANGLE)), "");
        let path = write(dir.path(), "a.gltf", json.as_bytes());
        let model = load(&path).unwrap();
        assert_eq!(model.meshes.len(), 1);
        let mesh = &model.meshes[0];
        assert_eq!(mesh.vertices[1].position, Vec3::new(3.0, 2.0, 3.0));
        assert_eq!(mesh.vertices[2].position, Vec3::new(1.0, 4.0, 3.0));
        assert!(mesh.vertices.iter().all(|v| v.normal == Vec3::Z));
        assert_eq!(mesh.indices, [0, 1, 2]);
        assert!(mesh.needs_tangents);
        // No material: the mesh defaults.
        assert_eq!(mesh.roughness_factor, 0.5);
    }

    #[test]
    fn lines_are_skipped() {
        let dir = tempfile::tempdir().unwrap();
        let json = gltf_json(&data_uri(&floats(&TRIANGLE)), r#","mode":1"#);
        let path = write(dir.path(), "a.gltf", json.as_bytes());
        assert!(load(&path).unwrap().meshes.is_empty());
    }

    #[test]
    fn index_past_the_end_is_an_error() {
        let dir = tempfile::tempdir().unwrap();
        let mut bytes = floats(&TRIANGLE);
        bytes.extend([0u16, 1, 7].iter().flat_map(|v| v.to_le_bytes()));
        let json = format!(
            r#"{{"asset":{{"version":"2.0"}},
            "buffers":[{{"byteLength":42,"uri":"{}"}}],
            "bufferViews":[{{"buffer":0,"byteLength":36}},{{"buffer":0,"byteOffset":36,"byteLength":6}}],
            "accessors":[
              {{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]}},
              {{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}}],
            "meshes":[{{"primitives":[{{"attributes":{{"POSITION":0}},"indices":1}}]}}],
            "nodes":[{{"mesh":0}}]}}"#,
            data_uri(&bytes)
        );
        let path = write(dir.path(), "a.gltf", json.as_bytes());
        assert!(load(&path).is_err());
    }

    #[test]
    fn images_decode_once_and_failures_drop() {
        let dir = tempfile::tempdir().unwrap();
        let good = png_bytes(2, 1, &[255, 0, 0, 255, 0, 255, 0, 255]);
        let json = format!(
            r#"{{"asset":{{"version":"2.0"}},
            "buffers":[{{"byteLength":36,"uri":"{}"}}],
            "bufferViews":[{{"buffer":0,"byteLength":36}}],
            "accessors":[{{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3",
                "min":[0,0,0],"max":[1,1,0]}}],
            "images":[{{"uri":"tex.png"}},{{"uri":"tex.png"}},{{"uri":"bad.png"}}],
            "textures":[{{"source":0}},{{"source":1}},{{"source":2}}],
            "materials":[
              {{"pbrMetallicRoughness":{{"baseColorTexture":{{"index":0,
                  "extensions":{{"KHR_texture_transform":{{"offset":[0.5,0.25],"scale":[2,3]}}}}}},
                 "metallicFactor":0.25}},
               "alphaMode":"MASK","doubleSided":true,
               "emissiveTexture":{{"index":2}}}},
              {{"normalTexture":{{"index":1}},
               "extensions":{{"KHR_materials_clearcoat":{{"clearcoatFactor":0.75}},
                 "KHR_materials_diffuse_transmission":{{"diffuseTransmissionFactor":0.5,
                   "diffuseTransmissionColorFactor":[0.1,0.2,0.3]}}}}}}],
            "meshes":[{{"primitives":[
              {{"attributes":{{"POSITION":0}},"material":0}},
              {{"attributes":{{"POSITION":0}},"material":1}}]}}],
            "nodes":[{{"mesh":0}}]}}"#,
            data_uri(&floats(&TRIANGLE))
        );
        write(dir.path(), "tex.png", &good);
        write(dir.path(), "bad.png", b"not an image");
        let path = write(dir.path(), "a.gltf", json.as_bytes());
        let model = load(&path).unwrap();

        // tex.png is listed twice and decodes once; bad.png is gone.
        assert_eq!(model.images.len(), 1);
        assert_eq!(
            (model.images[0].rgba.width, model.images[0].rgba.height),
            (2, 1)
        );
        let [first, second] = &model.meshes[..] else {
            panic!("two meshes")
        };
        assert_eq!(first.textures.albedo, Some(0));
        assert_eq!(first.textures.emissive, None);
        assert_eq!(second.textures.normal, Some(0));
        assert_eq!(first.uv_transforms.albedo.offset, [0.5, 0.25]);
        assert_eq!(first.uv_transforms.albedo.scale, [2.0, 3.0]);
        assert_eq!(first.uv_transforms.normal, IDENTITY);
        assert_eq!(first.metallic_factor, 0.25);
        assert_eq!(first.alpha_mode, AlphaMode::Mask);
        assert_eq!(first.alpha_cutoff, 0.5);
        assert!(first.double_sided);
        assert_eq!(second.clearcoat_factor, 0.75);
        assert_eq!(second.diffuse_transmission, [0.1, 0.2, 0.3, 0.5]);
    }

    #[test]
    fn glb_binary_chunk_and_authored_tangents() {
        let mut bin = floats(&TRIANGLE);
        bin.extend(floats(&[0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0]));
        bin.extend(floats(&[
            1.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, -1.0, 1.0, 0.0, 0.0, -1.0,
        ]));
        let json = format!(
            r#"{{"asset":{{"version":"2.0"}},
            "buffers":[{{"byteLength":{}}}],
            "bufferViews":[{{"buffer":0,"byteLength":36}},
              {{"buffer":0,"byteOffset":36,"byteLength":36}},
              {{"buffer":0,"byteOffset":72,"byteLength":48}}],
            "accessors":[
              {{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]}},
              {{"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"}},
              {{"bufferView":2,"componentType":5126,"count":3,"type":"VEC4"}}],
            "meshes":[{{"primitives":[{{"attributes":{{"POSITION":0,"NORMAL":1,"TANGENT":2}}}}]}}],
            "nodes":[{{"mesh":0}}]}}"#,
            bin.len()
        );
        let mut json = json.into_bytes();
        while json.len() % 4 != 0 {
            json.push(b' ');
        }
        let total = 12 + 8 + json.len() + 8 + bin.len();
        let mut glb = Vec::new();
        glb.extend(b"glTF");
        glb.extend(2u32.to_le_bytes());
        glb.extend((total as u32).to_le_bytes());
        glb.extend((json.len() as u32).to_le_bytes());
        glb.extend(b"JSON");
        glb.extend(&json);
        glb.extend((bin.len() as u32).to_le_bytes());
        glb.extend(b"BIN\0");
        glb.extend(&bin);

        let dir = tempfile::tempdir().unwrap();
        let path = write(dir.path(), "a.glb", &glb);
        let model = load(&path).unwrap();
        let mesh = &model.meshes[0];
        assert!(!mesh.needs_tangents);
        assert_eq!(mesh.vertices[0].normal, Vec3::Z);
        assert_eq!(mesh.vertices[0].tangent, Vec4::new(1.0, 0.0, 0.0, -1.0));
        assert_eq!(mesh.vertices.len(), 3);
    }

    #[test]
    fn dependencies_skip_data_uris_and_decode_names() {
        let dir = tempfile::tempdir().unwrap();
        let json = r#"{"asset":{"version":"2.0"},
            "buffers":[{"byteLength":4,"uri":"data:application/octet-stream;base64,AAAAAA=="},
                       {"byteLength":4,"uri":"geometry.bin"}],
            "images":[{"uri":"my%20tex.png"},{"uri":"data:image/png;base64,AAAA"}]}"#;
        let path = write(dir.path(), "a.gltf", json.as_bytes());
        assert_eq!(
            dependencies(&path),
            [
                dir.path().join("geometry.bin"),
                dir.path().join("my tex.png")
            ]
        );
        assert!(dependencies(&dir.path().join("missing.gltf")).is_empty());
    }

    /// Positions as i16 and normals as normalized i8, which
    /// `KHR_mesh_quantization` allows and the `gltf` crate does not read.
    #[test]
    fn quantized_attributes() {
        let mut bin: Vec<u8> = [0i16, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0]
            .iter()
            .flat_map(|v| v.to_le_bytes())
            .collect();
        bin.extend([0i8, 0, 127, 0, 0, 0, 127, 0, 0, 0, 127, 0].map(|v| v as u8));
        let json = format!(
            r#"{{"asset":{{"version":"2.0"}},"extensionsRequired":["KHR_mesh_quantization"],
            "extensionsUsed":["KHR_mesh_quantization"],
            "buffers":[{{"byteLength":{len},"uri":"{uri}"}}],
            "bufferViews":[{{"buffer":0,"byteLength":24,"byteStride":8}},
              {{"buffer":0,"byteOffset":24,"byteLength":12,"byteStride":4}}],
            "accessors":[
              {{"bufferView":0,"componentType":5122,"count":3,"type":"VEC3","min":[0,0,0],"max":[2,2,0]}},
              {{"bufferView":1,"componentType":5120,"normalized":true,"count":3,"type":"VEC3"}}],
            "meshes":[{{"primitives":[{{"attributes":{{"POSITION":0,"NORMAL":1}}}}]}}],
            "nodes":[{{"mesh":0}}]}}"#,
            len = bin.len(),
            uri = data_uri(&bin)
        );
        let dir = tempfile::tempdir().unwrap();
        let path = write(dir.path(), "a.gltf", json.as_bytes());
        let model = load(&path).unwrap();
        let mesh = &model.meshes[0];
        assert_eq!(mesh.vertices[1].position, Vec3::new(0.0, 2.0, 0.0));
        assert_eq!(mesh.vertices[2].position, Vec3::new(2.0, 0.0, 0.0));
        assert!(mesh.vertices.iter().all(|v| v.normal == Vec3::Z));
        assert!(mesh.needs_tangents);
    }

    #[test]
    fn a_required_extension_that_changes_the_data_is_refused() {
        let json = r#"{"asset":{"version":"2.0"},
            "extensionsRequired":["EXT_meshopt_compression"]}"#;
        let error = parse(json.as_bytes()).err().unwrap();
        assert!(error.to_string().contains("EXT_meshopt_compression"));
    }

    #[test]
    fn percent_escapes() {
        assert_eq!(percent_decode("a%20b%2Fc%zz%"), "a b/c%zz%");
    }
}
