//! Up-to-date checks, the clean-up of a failed output, and the removal of
//! outputs the manifest does not list.

use crate::progress::Progress;
use anyhow::{Context, Result, bail};
use rayon::prelude::*;
use std::collections::BTreeSet;
use std::fs;
use std::path::{Path, PathBuf};

type Run<'a> = Box<dyn FnOnce(&Path) -> Result<()> + Send + 'a>;

/// One output to bake: its name under the output folder, the files it reads,
/// and the conversion that writes it.
pub struct Job<'a> {
    pub name: String,
    pub inputs: Vec<PathBuf>,
    pub run: Run<'a>,
}

impl<'a> Job<'a> {
    pub fn new(
        name: &str,
        inputs: Vec<PathBuf>,
        run: impl FnOnce(&Path) -> Result<()> + Send + 'a,
    ) -> Self {
        Self {
            name: name.to_string(),
            inputs,
            run: Box::new(run),
        }
    }
}

pub struct Baker {
    manifest: PathBuf,
    converter: PathBuf,
    source_root: PathBuf,
    output_root: PathBuf,
    outputs: BTreeSet<PathBuf>,
}

impl Baker {
    /// Sources are relative to the manifest's folder, outputs to `output_root`.
    pub fn new(manifest: &Path, output_root: &Path) -> Result<Self> {
        Ok(Self {
            manifest: manifest.to_path_buf(),
            // Every output also depends on the converter, which stands in for
            // the format code compiled into it.
            converter: std::env::current_exe().context("unable to find the yeschef executable")?,
            source_root: manifest.parent().unwrap_or(Path::new("")).to_path_buf(),
            output_root: output_root.to_path_buf(),
            outputs: BTreeSet::new(),
        })
    }

    pub fn source(&self, relative: &str) -> PathBuf {
        self.source_root.join(relative)
    }

    /// Bakes every job on the rayon pool, so assets run side by side and a
    /// converter that splits its own work shares the same threads. Every job
    /// runs even if one fails. Errors come back in job order, so the report
    /// does not depend on the thread count or on timing.
    pub fn run(&mut self, jobs: Vec<Job<'_>>) -> Result<()> {
        for job in &jobs {
            self.outputs.insert(self.output_root.join(&job.name));
        }
        let baker = &*self;
        let progress = Progress::new(
            jobs.len(),
            vec![self.source_root.clone(), self.output_root.clone()],
        );
        let progress = &progress;
        let results: Vec<Result<()>> = jobs
            .into_par_iter()
            .map(|job| {
                let name = job.name.clone();
                baker.bake(job, progress).with_context(|| name)
            })
            .collect();
        let mut failed = 0;
        for error in results.into_iter().filter_map(Result::err) {
            eprintln!("yeschef: {error:#}");
            failed += 1;
        }
        if failed > 0 {
            bail!("{failed} of the assets failed");
        }
        Ok(())
    }

    /// Runs the job unless its output is newer than its inputs, the manifest
    /// and the converter. A failed run deletes its output, so a partial file
    /// never looks current.
    fn bake(&self, job: Job<'_>, progress: &Progress) -> Result<()> {
        let output = self.output_root.join(&job.name);
        let mut inputs = job.inputs;
        inputs.push(self.manifest.clone());
        inputs.push(self.converter.clone());
        if up_to_date(&output, &inputs) {
            progress.skip();
            return Ok(());
        }
        progress.start(&job.name);
        if let Some(parent) = output.parent() {
            fs::create_dir_all(parent)
                .with_context(|| format!("unable to create {}", parent.display()))?;
        }
        let result = (job.run)(&output).inspect_err(|_| {
            let _ = fs::remove_file(&output);
        });
        match result {
            Ok(()) => progress.done(&job.name),
            Err(_) => progress.failed(&job.name),
        }
        result
    }

    /// Deletes the files in the output folder that no bake listed, so an entry
    /// removed from the manifest does not ship. Call it only after every bake
    /// succeeded, so a failed run keeps them. The folder holds nothing but bake
    /// outputs. Returns what it deleted, sorted.
    pub fn sweep_orphans(&self) -> Result<Vec<PathBuf>> {
        if !self.output_root.is_dir() {
            return Ok(Vec::new());
        }
        let listed: BTreeSet<PathBuf> = self.outputs.iter().map(|path| normalize(path)).collect();
        let mut files = Vec::new();
        collect_files(&self.output_root, &mut files)?;
        files.sort();
        let mut removed = Vec::new();
        for file in files {
            if !listed.contains(&normalize(&file)) {
                fs::remove_file(&file)
                    .with_context(|| format!("unable to remove {}", file.display()))?;
                removed.push(file);
            }
        }
        Ok(removed)
    }
}

/// Current when newer than every input. A missing file on either side is
/// stale.
pub fn up_to_date(output: &Path, inputs: &[PathBuf]) -> bool {
    let Ok(built) = fs::metadata(output).and_then(|m| m.modified()) else {
        return false;
    };
    inputs.iter().all(|input| {
        fs::metadata(input)
            .and_then(|m| m.modified())
            .is_ok_and(|changed| changed <= built)
    })
}

/// The canonical form of a path, or the path itself if it does not exist.
fn normalize(path: &Path) -> PathBuf {
    fs::canonicalize(path).unwrap_or_else(|_| path.to_path_buf())
}

pub fn collect_files(folder: &Path, files: &mut Vec<PathBuf>) -> Result<()> {
    for entry in
        fs::read_dir(folder).with_context(|| format!("unable to read {}", folder.display()))?
    {
        let entry = entry?;
        if entry.file_type()?.is_dir() {
            collect_files(&entry.path(), files)?;
        } else if entry.file_type()?.is_file() {
            files.push(entry.path());
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::time::{Duration, SystemTime};

    fn write_at(path: &Path, offset_seconds: i64) {
        fs::create_dir_all(path.parent().unwrap()).unwrap();
        fs::write(path, "x").unwrap();
        let now = SystemTime::now();
        let time = if offset_seconds >= 0 {
            now + Duration::from_secs(offset_seconds as u64)
        } else {
            now - Duration::from_secs(offset_seconds.unsigned_abs())
        };
        fs::File::options()
            .write(true)
            .open(path)
            .unwrap()
            .set_modified(time)
            .unwrap();
    }

    fn baker(dir: &Path) -> Baker {
        let manifest = dir.join("assets/manifest.yaml");
        write_at(&manifest, -3600);
        Baker::new(&manifest, &dir.join("out")).unwrap()
    }

    fn job<'a>(name: &str, run: impl FnOnce(&Path) -> Result<()> + Send + 'a) -> Job<'a> {
        Job::new(name, vec![], run)
    }

    #[test]
    fn newer_than_every_input_is_current() {
        let dir = tempfile::tempdir().unwrap();
        let (output, source) = (dir.path().join("out.bin"), dir.path().join("in.bin"));
        assert!(
            !up_to_date(&output, std::slice::from_ref(&source)),
            "missing output"
        );
        write_at(&output, -10);
        assert!(
            !up_to_date(&output, std::slice::from_ref(&source)),
            "missing input"
        );
        write_at(&source, -20);
        assert!(up_to_date(&output, std::slice::from_ref(&source)));
        write_at(&source, 20);
        assert!(!up_to_date(&output, &[source]), "input newer than output");
    }

    #[test]
    fn a_current_output_is_not_rebuilt() {
        let dir = tempfile::tempdir().unwrap();
        let mut baker = baker(dir.path());
        let runs = AtomicUsize::new(0);
        for _ in 0..2 {
            baker
                .run(vec![job("a.bin", |to| {
                    runs.fetch_add(1, Ordering::SeqCst);
                    write_at(to, 3600);
                    Ok(())
                })])
                .unwrap();
        }
        assert_eq!(runs.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn a_changed_manifest_rebuilds() {
        let dir = tempfile::tempdir().unwrap();
        let mut baker = baker(dir.path());
        write_at(&dir.path().join("out/a.bin"), -7200);
        let runs = AtomicUsize::new(0);
        baker
            .run(vec![job("a.bin", |_| {
                runs.fetch_add(1, Ordering::SeqCst);
                Ok(())
            })])
            .unwrap();
        assert_eq!(
            runs.load(Ordering::SeqCst),
            1,
            "the manifest is newer than the output"
        );
    }

    #[test]
    fn a_failed_run_deletes_its_output() {
        let dir = tempfile::tempdir().unwrap();
        let mut baker = baker(dir.path());
        let result = baker.run(vec![job("a.bin", |to| {
            fs::write(to, "partial").unwrap();
            anyhow::bail!("broken")
        })]);
        assert!(result.is_err());
        assert!(!dir.path().join("out/a.bin").exists());
    }

    #[test]
    fn one_failure_does_not_stop_the_other_jobs() {
        let dir = tempfile::tempdir().unwrap();
        let mut baker = baker(dir.path());
        let jobs = (0..16)
            .map(|i| {
                job(&format!("{i}.bin"), move |to| {
                    if i == 3 {
                        anyhow::bail!("broken")
                    }
                    write_at(to, 3600);
                    Ok(())
                })
            })
            .collect();
        let error = baker.run(jobs).unwrap_err();
        assert!(error.to_string().contains("1 of the assets failed"));
        for i in (0..16).filter(|&i| i != 3) {
            assert!(dir.path().join(format!("out/{i}.bin")).exists());
        }
    }

    #[test]
    fn the_sweep_deletes_only_unlisted_files() {
        let dir = tempfile::tempdir().unwrap();
        let mut baker = baker(dir.path());
        baker
            .run(vec![job("models/a.bin", |to| {
                write_at(to, 3600);
                Ok(())
            })])
            .unwrap();
        let orphan = dir.path().join("out/old/b.bin");
        write_at(&orphan, 0);
        let removed = baker.sweep_orphans().unwrap();
        assert_eq!(removed, vec![orphan.clone()]);
        assert!(!orphan.exists());
        assert!(dir.path().join("out/models/a.bin").exists());
    }

    #[test]
    fn the_sweep_ignores_a_missing_folder() {
        let dir = tempfile::tempdir().unwrap();
        let baker = baker(dir.path());
        assert!(baker.sweep_orphans().unwrap().is_empty());
    }
}
