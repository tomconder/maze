//! yeschef, the build-time asset converter. It takes the command line of
//! `assetconv` and will replace it.

mod atlas;
mod bake;
mod darken;
mod font;
mod hdr;
mod image;
mod import;
mod manifest;
mod mip;
mod model;
mod notices;
mod pack;
mod shaders;
mod texture;

use anyhow::Result;
use bake::{Baker, Job, collect_files};
use clap::{ArgGroup, Parser};
use manifest::Manifest;
use notices::{Licenses, write_notices};
use std::collections::BTreeSet;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

/// Bakes the models, textures, fonts, atlases and shaders a manifest lists.
#[derive(Parser)]
#[command(version, group(ArgGroup::new("mode").required(true).args(["manifest", "verify"])))]
struct Cli {
    /// Threads for conversion. The default is one per hardware thread.
    #[arg(long, value_name = "N", value_parser = clap::value_parser!(u32).range(1..))]
    threads: Option<u32>,

    /// Bake every asset the manifest lists. An output is skipped when it is
    /// newer than its sources, the manifest and this program. Files in the
    /// output directory that the manifest does not list are deleted.
    #[arg(long, num_args = 2, value_names = ["MANIFEST", "OUTPUT_DIR"])]
    manifest: Option<Vec<PathBuf>>,

    /// Import SOURCE again and compare it with the baked OUTPUT.
    #[arg(long, num_args = 2, value_names = ["SOURCE", "OUTPUT"])]
    verify: Option<Vec<PathBuf>>,

    /// Leave `#line` directives out of the shader pack.
    #[arg(long, requires = "manifest")]
    no_line_directives: bool,

    /// Skip the models marked `debug: true`.
    #[arg(long, requires = "manifest")]
    no_debug_assets: bool,

    /// Join the third-party licenses into FILE: the manifest's `licenses`
    /// plus each --license.
    #[arg(long, value_name = "FILE", requires = "manifest")]
    notices: Option<PathBuf>,

    /// A license for the notices file. A name given more than once gets all
    /// its files in one section.
    #[arg(long = "license", value_name = "NAME=PATH", requires = "notices", value_parser = parse_license)]
    licenses: Vec<(String, PathBuf)>,
}

fn parse_license(value: &str) -> Result<(String, PathBuf), String> {
    value
        .split_once('=')
        .map(|(name, path)| (name.to_string(), PathBuf::from(path)))
        .ok_or_else(|| format!("expected NAME=PATH, got {value}"))
}

/// Why a run failed. A manifest that cannot be read is exit code 2, like a
/// bad command line. A conversion that fails is exit code 1.
enum Failure {
    Manifest(anyhow::Error),
    Bake(anyhow::Error),
}

fn main() -> ExitCode {
    let cli = Cli::parse();
    // One pool for the whole run. Without --threads rayon uses one thread per
    // hardware thread.
    if let Some(threads) = cli.threads
        && let Err(error) = rayon::ThreadPoolBuilder::new()
            .num_threads(threads as usize)
            .build_global()
    {
        eprintln!("yeschef: {error}");
        return ExitCode::FAILURE;
    }
    let outcome = match (&cli.manifest, &cli.verify) {
        (Some(paths), _) => bake_manifest(&cli, &paths[0], &paths[1]),
        (_, Some(paths)) => model::verify(&paths[0], &paths[1]).map_err(Failure::Bake),
        _ => unreachable!("clap requires one mode"),
    };
    match outcome {
        Ok(()) => ExitCode::SUCCESS,
        Err(Failure::Manifest(error)) => {
            eprintln!("yeschef: {error:#}");
            ExitCode::from(2)
        }
        Err(Failure::Bake(error)) => {
            eprintln!("yeschef: {error:#}");
            ExitCode::FAILURE
        }
    }
}

fn bake_manifest(cli: &Cli, manifest_path: &Path, output_dir: &Path) -> Result<(), Failure> {
    let manifest = Manifest::load(manifest_path)
        .map_err(|error| Failure::Manifest(error.context(manifest_path.display().to_string())))?;
    let mut baker = Baker::new(manifest_path, output_dir).map_err(Failure::Bake)?;

    bake_all(
        &mut baker,
        &manifest,
        !cli.no_line_directives,
        !cli.no_debug_assets,
    )
    .map_err(Failure::Bake)?;

    for orphan in baker.sweep_orphans().map_err(Failure::Bake)? {
        println!(
            "removed orphan {}",
            orphan.to_string_lossy().replace('\\', "/")
        );
    }

    // Asset licenses come from the manifest and code licenses from the command
    // line, because only the build knows what the game links.
    if let Some(notices) = &cli.notices {
        let mut licenses = Licenses::new();
        for (name, path) in &cli.licenses {
            licenses.entry(name.clone()).or_default().push(path.clone());
        }
        for license in &manifest.licenses {
            let files = licenses.entry(license.name.clone()).or_default();
            files.extend(license.files.iter().map(|file| baker.source(file)));
        }
        if let Some(bytes) = write_notices(notices, &licenses).map_err(Failure::Bake)? {
            println!(
                "{} licenses -> {} ({bytes} bytes)",
                licenses.len(),
                notices.display()
            );
        }
    }
    Ok(())
}

/// Lists a job per output in the order the sections appear in the manifest,
/// then runs them all on the pool.
fn bake_all(
    baker: &mut Baker,
    manifest: &Manifest,
    line_directives: bool,
    debug_assets: bool,
) -> Result<()> {
    let mut jobs = Vec::new();

    for model in &manifest.models {
        if !debug_assets && model.debug {
            continue;
        }
        let from = baker.source(&model.source);
        // External buffers and images are inputs too.
        let mut inputs = vec![from.clone()];
        inputs.extend(import::dependencies(&from));
        jobs.push(Job::new(&model.output, inputs, move |to| {
            model::convert(&from, to)
        }));
    }

    for texture in &manifest.textures {
        let from = baker.source(&texture.source);
        jobs.push(Job::new(&texture.output, vec![from.clone()], move |to| {
            texture::convert(&from, to)
        }));
    }

    for environment in &manifest.environments {
        let from = baker.source(&environment.source);
        jobs.push(Job::new(
            &environment.output,
            vec![from.clone()],
            move |to| hdr::convert(&from, to),
        ));
    }

    for font in &manifest.fonts {
        let from = baker.source(&font.source);
        jobs.push(Job::new(&font.output, vec![from.clone()], move |to| {
            font::convert(&from, to, &font.sizes)
        }));
    }

    for atlas in &manifest.atlases {
        let sprites: Vec<(String, PathBuf)> = atlas
            .sprites
            .iter()
            .map(|(name, path)| (name.clone(), baker.source(path)))
            .collect();
        let inputs = sprites.iter().map(|(_, path)| path.clone()).collect();
        jobs.push(Job::new(&atlas.output, inputs, move |to| {
            atlas::convert(&sprites, to)
        }));
    }

    if let Some(shaders) = &manifest.shaders {
        let stages: Vec<_> = shaders
            .stages()?
            .into_iter()
            .map(|stage| {
                let path = baker.source(stage.source);
                (stage, path)
            })
            .collect();

        // Includes are not listed anywhere, so every file under a stage's
        // folder counts as an input.
        let folders: BTreeSet<&Path> = stages
            .iter()
            .filter_map(|(_, path)| path.parent())
            .collect();
        let mut inputs = Vec::new();
        for folder in folders {
            collect_files(folder, &mut inputs)?;
        }
        jobs.push(Job::new(&shaders.output, inputs, move |to| {
            shaders::pack(&stages, to, line_directives)
        }));
    }

    baker.run(jobs)
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::CommandFactory;

    #[test]
    fn the_command_line_is_consistent() {
        Cli::command().debug_assert();
    }

    #[test]
    fn a_license_needs_a_name_and_a_path() {
        assert_eq!(
            parse_license("zlib=a/b.txt").unwrap(),
            ("zlib".to_string(), PathBuf::from("a/b.txt"))
        );
        assert!(parse_license("zlib").is_err());
    }
}
