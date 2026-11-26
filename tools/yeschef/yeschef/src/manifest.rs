//! The asset manifest, `assets/manifest.yaml`.
//!
//! Paths are kept as written. Sources are relative to the manifest's folder
//! and outputs to the output directory. A misspelled key is an error, not a
//! silently skipped asset. The atlas sprites and the shader stages keep their
//! file order, because the atlas and the shader pack lay out their entries in
//! that order. A font can name a saved atlas, which the bake uses in place of
//! the one it draws.

use anyhow::{Context, Result, bail};
use indexmap::IndexMap;
use serde::Deserialize;
use std::path::Path;
use yeschef_format::archive::ARCHIVE_ALIGNMENT;

#[derive(Debug, Default, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Manifest {
    #[serde(default)]
    pub models: Vec<Model>,
    #[serde(default)]
    pub textures: Vec<Image>,
    #[serde(default)]
    pub environments: Vec<Image>,
    #[serde(default)]
    pub fonts: Vec<Font>,
    #[serde(default)]
    pub atlases: Vec<Atlas>,
    #[serde(default)]
    pub scenes: Vec<Verbatim>,
    #[serde(default)]
    pub audio: Vec<Verbatim>,
    #[serde(default)]
    pub licenses: Vec<License>,
    pub shaders: Option<Shaders>,
    #[serde(default)]
    pub pack: Pack,
}

/// How `--pack` splits the asset pack.
#[derive(Debug, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct Pack {
    /// The largest data file, in bytes. A larger entry has a data file of its
    /// own.
    pub max_file_size: u64,
}

impl Default for Pack {
    fn default() -> Self {
        Self {
            max_file_size: 1 << 30,
        }
    }
}

/// The header and one aligned entry need this many bytes.
const MIN_PACK_FILE_SIZE: u64 = 2 * ARCHIVE_ALIGNMENT;

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Model {
    pub source: String,
    pub output: String,
    /// Only debug builds use the model.
    #[serde(default)]
    pub debug: bool,
}

/// A scene or audio file. The bake copies it unchanged, so that it is in the
/// pack.
#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Verbatim {
    pub source: String,
    pub output: String,
    /// Only debug builds use the file.
    #[serde(default)]
    pub debug: bool,
}

/// A texture or an environment map: one source image, one output.
#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Image {
    pub source: String,
    pub output: String,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Font {
    pub source: String,
    pub output: String,
    pub sizes: Vec<u32>,
    pub atlas: Option<SavedAtlas>,
}

/// An edited font atlas that is kept with the sources.
#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct SavedAtlas {
    /// A KTX2 file.
    pub file: String,
    /// The layout ID `--extract-atlas` printed for the font the atlas came
    /// from. The bake fails when the font no longer has it.
    pub layout: String,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Atlas {
    pub output: String,
    /// Sprite name to image. The name is what the engine looks up.
    pub sprites: IndexMap<String, String>,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct License {
    pub name: String,
    pub files: Vec<String>,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Shaders {
    pub output: String,
    /// Name the engine asks for to `<slang source>:<entry point>`.
    pub stages: IndexMap<String, String>,
}

/// A shader stage split at its last colon.
#[derive(Debug, PartialEq, Eq)]
pub struct ShaderStage<'a> {
    pub name: &'a str,
    pub source: &'a str,
    pub entry_point: &'a str,
}

impl Shaders {
    pub fn stages(&self) -> Result<Vec<ShaderStage<'_>>> {
        self.stages
            .iter()
            .map(|(name, value)| match value.rsplit_once(':') {
                Some((source, entry_point)) => Ok(ShaderStage {
                    name,
                    source,
                    entry_point,
                }),
                None => bail!("stage {name} is not <slang>:<entry>, got {value}"),
            })
            .collect()
    }
}

impl Manifest {
    pub fn parse(text: &str) -> Result<Self> {
        let manifest: Manifest = serde_saphyr::from_str(text)?;
        if let Some(shaders) = &manifest.shaders {
            shaders.stages()?;
        }
        let cap = manifest.pack.max_file_size;
        if cap < MIN_PACK_FILE_SIZE {
            bail!("pack max_file_size is {cap}, less than {MIN_PACK_FILE_SIZE}");
        }
        Ok(manifest)
    }

    pub fn load(path: &Path) -> Result<Self> {
        let text = std::fs::read_to_string(path)
            .with_context(|| format!("unable to read {}", path.display()))?;
        Self::parse(&text)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_the_game_manifest() {
        let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/manifest.yaml");
        let manifest = Manifest::load(&path).unwrap();
        assert!(!manifest.models.is_empty());
        assert!(manifest.models.iter().any(|m| m.debug));
        assert!(!manifest.fonts[0].sizes.is_empty());
        assert!(!manifest.licenses.is_empty());
        assert!(manifest.scenes.iter().any(|s| !s.debug));
        assert!(manifest.scenes.iter().any(|s| s.debug));
        assert!(!manifest.audio.is_empty());

        // File order survives the parse.
        let atlas = &manifest.atlases[0];
        assert_eq!(
            atlas.sprites.keys().next().unwrap(),
            "keyboard_arrows_horizontal"
        );
        let shaders = manifest.shaders.unwrap();
        let stages = shaders.stages().unwrap();
        assert_eq!(stages[0].name, "bloom_down.comp");
        assert_eq!(
            stages[0],
            ShaderStage {
                name: "bloom_down.comp",
                source: "shaders/slang/bloom.slang",
                entry_point: "downMain",
            }
        );
        assert!(stages.len() > 30);
    }

    #[test]
    fn a_missing_section_is_empty() {
        let manifest = Manifest::parse("licenses: []\n").unwrap();
        assert!(manifest.models.is_empty());
        assert!(manifest.shaders.is_none());
    }

    #[test]
    fn the_pack_cap_defaults_to_1_gib_and_has_a_minimum() {
        let cap = |text: &str| Manifest::parse(text).map(|m| m.pack.max_file_size);
        assert_eq!(cap("licenses: []\n").unwrap(), 1 << 30);
        assert_eq!(cap("pack: {}\n").unwrap(), 1 << 30);
        assert_eq!(cap("pack:\n  max_file_size: 8192\n").unwrap(), 8192);
        let error = cap("pack:\n  max_file_size: 8191\n").unwrap_err();
        assert!(error.to_string().contains("less than 8192"), "{error}");
        assert!(cap("pack:\n  max_size: 8192\n").is_err(), "misspelled key");
    }

    #[test]
    fn a_saved_atlas_needs_a_file_and_a_layout() {
        let font = "fonts:\n  - source: a.ttf\n    output: a.spnga\n    sizes: [12]\n";
        assert!(Manifest::parse(font).unwrap().fonts[0].atlas.is_none());

        let saved = format!("{font}    atlas:\n      file: a.ktx2\n      layout: \"0012\"\n");
        let manifest = Manifest::parse(&saved).unwrap();
        let atlas = manifest.fonts[0].atlas.as_ref().unwrap();
        assert_eq!((&*atlas.file, &*atlas.layout), ("a.ktx2", "0012"));

        let no_layout = format!("{font}    atlas:\n      file: a.ktx2\n");
        assert!(Manifest::parse(&no_layout).is_err());
    }

    #[test]
    fn a_misspelled_key_is_an_error() {
        assert!(Manifest::parse("models:\n  - src: a.glb\n    output: a.spnga\n").is_err());
        assert!(Manifest::parse("model: []\n").is_err());
    }

    #[test]
    fn a_stage_needs_an_entry_point() {
        let text = "shaders:\n  output: s.spnga\n  stages:\n    a.vert: a.slang\n";
        assert!(
            Manifest::parse(text)
                .unwrap_err()
                .to_string()
                .contains("a.vert")
        );
    }

    #[test]
    fn a_repeated_stage_name_is_not_dropped() {
        // A repeated key must not silently drop a stage.
        let text = "shaders:\n  output: s.spnga\n  stages:\n    a.vert: a.slang:one\n    a.vert: b.slang:two\n";
        if let Ok(manifest) = Manifest::parse(text) {
            assert_eq!(manifest.shaders.unwrap().stages().unwrap().len(), 2);
        }
    }

    #[test]
    fn a_stage_splits_at_the_last_colon() {
        let text = "shaders:\n  output: s.spnga\n  stages:\n    a.vert: c:/a.slang:main\n";
        let shaders = Manifest::parse(text).unwrap().shaders.unwrap();
        let stage = &shaders.stages().unwrap()[0];
        assert_eq!(stage.source, "c:/a.slang");
        assert_eq!(stage.entry_point, "main");
    }
}
