//! Compiles the manifest's Slang stages to GLSL 450 and packs them.

use anyhow::{Context, Result, anyhow, bail};
use rayon::prelude::*;
use shader_slang as slang;
use shader_slang::Downcast;
use std::collections::BTreeMap;
use std::ffi::CString;
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use yeschef_format::shaderpack::{self, Sources};

use crate::manifest::ShaderStage;

/// Slang creates a global session in about 0.2 s and a session holds about
/// 170 MB, so past a few of them the start-up costs more than the extra
/// threads save. Measured with `assetconv` on 25 stages: 3 threads took 1.3 s,
/// 4 took 1.4 s and 8 took 1.8 s. Raise it when the shaders grow enough that
/// compiling outweighs the start-up.
const MAX_SESSIONS: usize = 3;

/// The stages of one source file. A module loads once and each of its stages
/// compiles from it.
struct Module<'a> {
    path: &'a Path,
    stages: Vec<(usize, &'a ShaderStage<'a>)>,
}

pub fn pack(
    stages: &[(ShaderStage<'_>, PathBuf)],
    output: &Path,
    line_directives: bool,
) -> Result<()> {
    let mut seen = BTreeMap::new();
    for (stage, _) in stages {
        if seen.insert(stage.name, ()).is_some() {
            bail!("shader name {} is used twice", stage.name);
        }
    }

    // First-seen order, so a run does not depend on a hash.
    let mut modules: Vec<Module> = Vec::new();
    for (i, (stage, path)) in stages.iter().enumerate() {
        match modules.iter_mut().find(|m| m.path == path) {
            Some(module) => module.stages.push((i, stage)),
            None => modules.push(Module {
                path,
                stages: vec![(i, stage)],
            }),
        }
    }

    // Includes resolve against the folder of the source file.
    let mut search: Vec<CString> = Vec::new();
    for (_, path) in stages {
        let folder = path.parent().unwrap_or(Path::new("."));
        let folder = CString::new(folder.to_string_lossy().as_bytes())?;
        if !search.contains(&folder) {
            search.push(folder);
        }
    }

    let code: Vec<Mutex<Option<String>>> = stages.iter().map(|_| Mutex::new(None)).collect();
    let next = AtomicUsize::new(0);
    let failed = AtomicBool::new(false);
    let workers = MAX_SESSIONS
        .min(rayon::current_num_threads())
        .min(modules.len());

    // A Slang session and what it loads belong to one thread, so each worker
    // makes its own.
    let results: Vec<Result<()>> = (0..workers)
        .into_par_iter()
        .map(|_| {
            let result = work(&modules, &search, line_directives, &next, &failed, &code);
            if result.is_err() {
                failed.store(true, Ordering::SeqCst);
            }
            result
        })
        .collect();
    if let Some(error) = results.into_iter().find_map(Result::err) {
        return Err(error);
    }

    let mut sources = Sources::new();
    for ((stage, _), code) in stages.iter().zip(code) {
        let code = code.into_inner().unwrap();
        sources.insert(stage.name.to_string(), code.expect("every stage compiled"));
    }
    let bytes = shaderpack::write(&sources).map_err(|error| anyhow!(error))?;
    std::fs::write(output, &bytes)
        .with_context(|| format!("unable to write {}", output.display()))?;
    println!(
        "{} shaders -> {} ({} bytes)",
        sources.len(),
        output.display(),
        bytes.len()
    );
    Ok(())
}

fn work(
    modules: &[Module],
    search: &[CString],
    line_directives: bool,
    next: &AtomicUsize,
    failed: &AtomicBool,
    code: &[Mutex<Option<String>>],
) -> Result<()> {
    // The session is made when there is work, so a worker that finds the queue
    // empty does not pay for one.
    let mut session = None;
    loop {
        if failed.load(Ordering::SeqCst) {
            return Ok(());
        }
        let index = next.fetch_add(1, Ordering::SeqCst);
        let Some(module) = modules.get(index) else {
            return Ok(());
        };
        if session.is_none() {
            session = Some(create_session(search, line_directives)?);
        }
        let (_, session) = session.as_ref().unwrap();

        let stem = module
            .path
            .file_stem()
            .and_then(|stem| stem.to_str())
            .context("a shader path has no name")?;
        let loaded = session
            .load_module(stem)
            .map_err(|error| anyhow!("{}: {error}", module.path.display()))?;
        for &(i, stage) in &module.stages {
            let text = compile(session, &loaded, stage)
                .with_context(|| format!("{} [{}]", module.path.display(), stage.entry_point))?;
            *code[i].lock().unwrap() = Some(text);
        }
    }
}

/// The global session must outlive the session made from it.
fn create_session(
    search: &[CString],
    line_directives: bool,
) -> Result<(slang::GlobalSession, slang::Session)> {
    let global = slang::GlobalSession::new().context("unable to create a Slang session")?;
    // Release drops #line so the shipped GLSL carries no source paths. The
    // options go on the target: on the session alone Slang ignores them.
    let options = slang::CompilerOptions::default()
        .matrix_layout_column(true)
        .line_directive_mode(if line_directives {
            slang::LineDirectiveMode::Default
        } else {
            slang::LineDirectiveMode::None
        });
    let target = slang::TargetDesc::default()
        .format(slang::CompileTarget::Glsl)
        .profile(global.find_profile("glsl_450"))
        .options(&options);
    let targets = [target];
    let paths: Vec<*const i8> = search.iter().map(|path| path.as_ptr()).collect();
    let desc = slang::SessionDesc::default()
        .targets(&targets)
        .search_paths(&paths)
        .options(&options);
    let session = global
        .create_session(&desc)
        .context("unable to create a Slang session")?;
    Ok((global, session))
}

fn compile(
    session: &slang::Session,
    module: &slang::Module,
    stage: &ShaderStage<'_>,
) -> Result<String> {
    let entry = module
        .find_entry_point_by_name(stage.entry_point)
        .with_context(|| format!("no entry point {}", stage.entry_point))?;
    let composed = session
        .create_composite_component_type(&[module.downcast().clone(), entry.downcast().clone()])
        .map_err(|error| anyhow!("{error}"))?;
    let linked = composed.link().map_err(|error| anyhow!("{error}"))?;
    let code = linked
        .entry_point_code(0, 0)
        .map_err(|error| anyhow!("{error}"))?;
    Ok(String::from_utf8_lossy(code.as_slice()).into_owned())
}
