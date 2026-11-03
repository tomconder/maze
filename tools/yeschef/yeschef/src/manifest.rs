//! The asset manifest, `assets/manifest.yaml`.
//!
//! Paths are kept as written. Sources are relative to the manifest's folder
//! and outputs to the output directory. A misspelled key is an error, not a
//! silently skipped asset. The atlas sprites and the shader stages keep their
//! file order, because the atlas and the shader pack lay out their entries in
//! that order.

use anyhow::{Context, Result, bail};
use indexmap::IndexMap;
use serde::Deserialize;
use std::path::Path;

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
    pub licenses: Vec<License>,
    pub shaders: Option<Shaders>,
}

#[derive(Debug, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Model {
    pub source: String,
    pub output: String,
    /// Only debug builds use the model.
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
    fn a_stage_splits_at_the_last_colon() {
        let text = "shaders:\n  output: s.spnga\n  stages:\n    a.vert: c:/a.slang:main\n";
        let shaders = Manifest::parse(text).unwrap().shaders.unwrap();
        let stage = &shaders.stages().unwrap()[0];
        assert_eq!(stage.source, "c:/a.slang");
        assert_eq!(stage.entry_point, "main");
    }
}
