//! Writes the asset pack, `assets.spnidx` and the `assets_NNN.spndat` files,
//! from the baked outputs. The layout is in `yeschef_format::archive`.
//!
//! The build ID is 64 bit FNV-1a over these bytes, for each entry in name
//! order: the name, the size as a little endian `u64`, then the content.

use crate::bake::{Baker, up_to_date};
use anyhow::{Context, Result, ensure};
use std::fs::{self, File};
use std::io::{self, BufWriter, Read, Write};
use std::path::{Path, PathBuf};
use yeschef_format::archive::{
    ARCHIVE_ALIGNMENT, ARCHIVE_COMPRESSION_NONE, ArchiveEntry, Index, data_file_name, data_header,
    read_index, write_index,
};
use yeschef_format::font::{hash, hash_from};

const INDEX_NAME: &str = "assets.spnidx";

/// Fails when `pack` is the bake output folder or is inside it, because the
/// orphan sweep would delete the pack. It makes both folders, so that the
/// operating system can resolve them.
pub fn check_folder(pack: &Path, output: &Path) -> Result<()> {
    let resolve = |folder: &Path| {
        fs::create_dir_all(folder)
            .and_then(|()| fs::canonicalize(folder))
            .with_context(|| format!("unable to create {}", folder.display()))
    };
    let output_root = resolve(output)?;
    ensure!(
        !resolve(pack)?.starts_with(output_root),
        "the pack folder {} is in the output folder {}",
        pack.display(),
        output.display()
    );
    Ok(())
}

/// Writes the pack of the baked outputs when a job ran or the pack in `out` is
/// not current. Returns the warnings of a write, or `None` for no write.
pub fn update(baker: &Baker, out: &Path, cap: u64) -> Result<Option<Vec<String>>> {
    let (root, names) = (baker.output_root(), baker.output_names()?);
    // A job can date its output in the past, as a stale saved atlas does, so
    // the dates alone do not show every change.
    if !baker.ran_any() && is_current(root, &names, out, &baker.shared_inputs()) {
        return Ok(None);
    }
    write(root, &names, out, cap).map(Some)
}

/// Packs the files `names` under `root` into `out`. `names` are normalized and
/// sorted. A data file is at most `cap` bytes, except that a larger entry has a
/// data file of its own. Returns warnings.
pub fn write(root: &Path, names: &[String], out: &Path, cap: u64) -> Result<Vec<String>> {
    let (mut index, warnings) = place(root, names, cap)?;

    index.build_id = hash(&[]);
    for (name, entry) in &index.entries {
        let path = root.join(name);
        let content =
            fs::read(&path).with_context(|| format!("unable to read {}", path.display()))?;
        for part in [name.as_bytes(), &entry.size.to_le_bytes(), &content] {
            index.build_id = hash_from(index.build_id, part);
        }
    }

    // From here until the rename there is no index, so a write that stops
    // leaves no pack that looks valid.
    fs::create_dir_all(out).with_context(|| format!("unable to create {}", out.display()))?;
    remove_old(out)?;
    for data_file in 0..index.data_file_sizes.len() as u32 {
        let path = out.join(data_file_name(data_file));
        write_data_file(root, &index, data_file, &path)
            .with_context(|| format!("unable to write {}", path.display()))?;
    }
    let (path, temporary) = (out.join(INDEX_NAME), out.join("assets.spnidx.tmp"));
    fs::write(&temporary, write_index(&index))
        .and_then(|()| fs::rename(&temporary, &path))
        .with_context(|| format!("unable to write {}", path.display()))?;
    Ok(warnings)
}

/// The index, less its build ID, from the sizes of the sources.
fn place(root: &Path, names: &[String], cap: u64) -> Result<(Index, Vec<String>)> {
    let mut index = Index::default();
    let mut warnings = Vec::new();
    for name in names {
        let path = root.join(name);
        let size = fs::metadata(&path)
            .with_context(|| format!("unable to read {}", path.display()))?
            .len();
        // The next free place in the last data file, if the entry fits there.
        let next = index
            .data_file_sizes
            .last()
            .map(|end| end.next_multiple_of(ARCHIVE_ALIGNMENT))
            .filter(|offset| offset + size <= cap);
        let offset = next.unwrap_or_else(|| {
            index.data_file_sizes.push(0);
            ARCHIVE_ALIGNMENT
        });
        if ARCHIVE_ALIGNMENT + size > cap {
            warnings.push(format!(
                "{name} is {size} bytes, past the pack cap of {cap}, so it has its own data file"
            ));
        }
        let data_file = index.data_file_sizes.len() - 1;
        index.data_file_sizes[data_file] = offset + size;
        let entry = ArchiveEntry {
            data_file: data_file as u32,
            compression: ARCHIVE_COMPRESSION_NONE,
            offset,
            size,
            raw_size: size,
            ..Default::default()
        };
        index.entries.push((name.clone(), entry));
    }
    Ok((index, warnings))
}

/// Deletes the index first, then every data file.
fn remove_old(out: &Path) -> Result<()> {
    let remove = |path: &Path| match fs::remove_file(path) {
        Err(error) if error.kind() != io::ErrorKind::NotFound => {
            Err(error).with_context(|| format!("unable to remove {}", path.display()))
        }
        _ => Ok(()),
    };
    remove(&out.join(INDEX_NAME))?;
    for entry in fs::read_dir(out).with_context(|| format!("unable to read {}", out.display()))? {
        let entry = entry?;
        let path = entry.path();
        if entry.file_type()?.is_file() && path.extension().is_some_and(|e| e == "spndat") {
            remove(&path)?;
        }
    }
    Ok(())
}

fn write_data_file(root: &Path, index: &Index, data_file: u32, path: &Path) -> Result<()> {
    let mut out = BufWriter::new(File::create(path)?);
    let header = data_header(data_file, index.build_id);
    out.write_all(&header)?;
    let mut end = header.len() as u64;
    for (name, entry) in &index.entries {
        if entry.data_file != data_file {
            continue;
        }
        io::copy(&mut io::repeat(0).take(entry.offset - end), &mut out)?;
        let copied = io::copy(&mut File::open(root.join(name))?, &mut out)?;
        ensure!(copied == entry.size, "{name} changed during the pack write");
        end = entry.offset + entry.size;
    }
    out.flush()?;
    Ok(())
}

/// True when the pack in `out` holds exactly `names`, its files have the
/// recorded sizes, and the index is newer than the sources and `other_inputs`.
pub fn is_current(root: &Path, names: &[String], out: &Path, other_inputs: &[PathBuf]) -> bool {
    let path = out.join(INDEX_NAME);
    let Some(index) = fs::read(&path)
        .ok()
        .and_then(|bytes| read_index(&bytes).ok())
    else {
        return false;
    };
    if !index.entries.iter().map(|(name, _)| name).eq(names) {
        return false;
    }
    let has_size = |file: PathBuf, size: u64| fs::metadata(file).is_ok_and(|m| m.len() == size);
    let mut inputs: Vec<PathBuf> = names.iter().map(|name| root.join(name)).collect();
    let entries_match = index
        .entries
        .iter()
        .zip(&inputs)
        .all(|((_, entry), source)| has_size(source.clone(), entry.size));
    let data_files_match = (0..)
        .zip(&index.data_file_sizes)
        .all(|(n, &size)| has_size(out.join(data_file_name(n)), size));
    inputs.extend_from_slice(other_inputs);
    entries_match && data_files_match && up_to_date(&path, &inputs)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bake::Job;
    use std::time::{Duration, SystemTime};

    const CAP: u64 = 16384;
    const HOUR: Duration = Duration::from_secs(3600);

    fn set_time(path: &Path, time: SystemTime) {
        File::options()
            .write(true)
            .open(path)
            .unwrap()
            .set_modified(time)
            .unwrap();
    }

    /// Files of the given sizes in `dir/root`, each dated an hour ago. Returns
    /// their names.
    fn sources(dir: &Path, files: &[(&str, usize)]) -> Vec<String> {
        fs::create_dir_all(dir.join("root")).unwrap();
        files
            .iter()
            .map(|&(name, size)| {
                let path = dir.join("root").join(name);
                fs::write(&path, vec![name.as_bytes()[0]; size]).unwrap();
                set_time(&path, SystemTime::now() - HOUR);
                name.to_string()
            })
            .collect()
    }

    fn pack(dir: &Path, names: &[String]) -> Vec<String> {
        write(&dir.join("root"), names, &dir.join("pack"), CAP).unwrap()
    }

    fn index(dir: &Path) -> Index {
        read_index(&fs::read(dir.join("pack").join(INDEX_NAME)).unwrap()).unwrap()
    }

    /// The name and bytes of every file in the pack folder, sorted.
    fn pack_files(dir: &Path) -> Vec<(String, Vec<u8>)> {
        let mut files: Vec<_> = fs::read_dir(dir.join("pack"))
            .unwrap()
            .map(|entry| {
                let entry = entry.unwrap();
                let name = entry.file_name().into_string().unwrap();
                (name, fs::read(entry.path()).unwrap())
            })
            .collect();
        files.sort();
        files
    }

    fn places(index: &Index) -> Vec<(u32, u64)> {
        index
            .entries
            .iter()
            .map(|(_, entry)| (entry.data_file, entry.offset))
            .collect()
    }

    /// Each entry in the pack equals its source, and each data file has the
    /// header and the size that the index gives.
    fn check_content(dir: &Path) {
        let index = index(dir);
        for (n, &size) in index.data_file_sizes.iter().enumerate() {
            let data = fs::read(dir.join("pack").join(data_file_name(n as u32))).unwrap();
            assert_eq!(data.len() as u64, size);
            let header = data_header(n as u32, index.build_id);
            assert_eq!(data[..header.len()], header);
        }
        for (name, entry) in &index.entries {
            let data = fs::read(dir.join("pack").join(data_file_name(entry.data_file))).unwrap();
            let stored = &data[entry.offset as usize..(entry.offset + entry.size) as usize];
            assert!(
                stored == fs::read(dir.join("root").join(name)).unwrap(),
                "{name}"
            );
        }
    }

    #[test]
    fn a_full_data_file_starts_the_next_one() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 5000), ("b", 5000), ("c", 5000)]);
        assert!(pack(dir.path(), &names).is_empty());
        assert_eq!(
            places(&index(dir.path())),
            [(0, 4096), (1, 4096), (2, 4096)]
        );
        check_content(dir.path());
    }

    #[test]
    fn entries_start_on_the_alignment_with_no_padding_at_the_end() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 3000), ("b", 3000), ("c", 3000)]);
        pack(dir.path(), &names);
        let index = index(dir.path());
        assert_eq!(places(&index), [(0, 4096), (0, 8192), (0, 12288)]);
        assert_eq!(index.data_file_sizes, [15288]);
        check_content(dir.path());
    }

    #[test]
    fn a_data_file_can_be_exactly_the_cap() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", CAP as usize - 4096)]);
        assert!(pack(dir.path(), &names).is_empty());
        assert_eq!(index(dir.path()).data_file_sizes, [CAP]);
    }

    #[test]
    fn a_large_entry_is_alone_in_its_data_file() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 10), ("b", 40000), ("c", 10)]);
        let warnings = pack(dir.path(), &names);
        assert_eq!(warnings.len(), 1, "{warnings:?}");
        assert!(warnings[0].starts_with("b is 40000 bytes"), "{warnings:?}");
        assert_eq!(
            places(&index(dir.path())),
            [(0, 4096), (1, 4096), (2, 4096)]
        );
        check_content(dir.path());
    }

    #[test]
    fn the_same_input_gives_the_same_files() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 3000), ("b", 9000)]);
        pack(dir.path(), &names);
        let first = pack_files(dir.path());
        pack(dir.path(), &names);
        assert!(pack_files(dir.path()) == first);

        // Other content gives another build ID.
        let id = index(dir.path()).build_id;
        fs::write(dir.path().join("root/b"), vec![0; 9000]).unwrap();
        pack(dir.path(), &names);
        assert_ne!(index(dir.path()).build_id, id);
    }

    #[test]
    fn the_build_id_is_the_documented_one() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 2)]);
        pack(dir.path(), &names);
        let bytes = [&b"a"[..], &2_u64.to_le_bytes(), b"aa"].concat();
        assert_eq!(index(dir.path()).build_id, hash(&bytes));
    }

    #[test]
    fn a_smaller_pack_leaves_no_old_data_file() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 5000), ("b", 5000), ("c", 5000)]);
        pack(dir.path(), &names);
        pack(dir.path(), &names[..1]);
        let left: Vec<_> = pack_files(dir.path()).into_iter().map(|f| f.0).collect();
        assert_eq!(left, ["assets.spnidx", "assets_000.spndat"]);
    }

    #[test]
    fn a_write_that_stops_leaves_no_index() {
        let dir = tempfile::tempdir().unwrap();
        let names = sources(dir.path(), &[("a", 10)]);
        pack(dir.path(), &names);
        // A folder with the name of a data file stops the write part way.
        fs::remove_file(dir.path().join("pack/assets_000.spndat")).unwrap();
        fs::create_dir(dir.path().join("pack/assets_000.spndat")).unwrap();
        assert!(
            write(
                &dir.path().join("root"),
                &names,
                &dir.path().join("pack"),
                CAP
            )
            .is_err()
        );
        assert!(!dir.path().join("pack").join(INDEX_NAME).exists());
    }

    #[test]
    fn no_entries_give_an_index_with_no_data_file() {
        let dir = tempfile::tempdir().unwrap();
        pack(dir.path(), &[]);
        assert!(index(dir.path()).data_file_sizes.is_empty());
        assert!(is_current(
            &dir.path().join("root"),
            &[],
            &dir.path().join("pack"),
            &[]
        ));
    }

    #[test]
    fn a_change_makes_the_pack_stale() {
        // Whether the pack is current after `change`, which gets the folder,
        // the names and the extra input.
        let current_after = |change: &dyn Fn(&Path, &mut Vec<String>, &Path)| {
            let dir = tempfile::tempdir().unwrap();
            let mut names = sources(dir.path(), &[("a", 3000), ("b", 3000)]);
            sources(dir.path(), &[("other", 1)]);
            let other = dir.path().join("root/other");
            pack(dir.path(), &names);
            change(dir.path(), &mut names, &other);
            is_current(
                &dir.path().join("root"),
                &names,
                &dir.path().join("pack"),
                &[other],
            )
        };
        let later = |path: PathBuf| set_time(&path, SystemTime::now() + HOUR);

        assert!(current_after(&|_, _, _| {}));
        assert!(
            !current_after(&|dir, _, _| later(dir.join("root/a"))),
            "changed entry with the same size"
        );
        assert!(
            !current_after(&|dir, _, _| {
                fs::write(dir.join("root/a"), "short").unwrap();
                set_time(&dir.join("root/a"), SystemTime::now() - HOUR);
            }),
            "changed entry size with an old date"
        );
        assert!(!current_after(&|_, _, other| later(other.to_path_buf())));
        assert!(
            !current_after(&|dir, names, _| {
                sources(dir, &[("c", 1)]);
                names.push("c".to_string());
            }),
            "new name"
        );
        assert!(
            !current_after(&|_, names, _| {
                names.pop();
            }),
            "removed name"
        );
        assert!(
            !current_after(&|dir, _, _| {
                fs::remove_file(dir.join("pack/assets_000.spndat")).unwrap();
            }),
            "missing data file"
        );
        assert!(
            !current_after(&|dir, _, _| {
                File::options()
                    .append(true)
                    .open(dir.join("pack/assets_000.spndat"))
                    .unwrap()
                    .write_all(b"x")
                    .unwrap();
            }),
            "data file with another size"
        );
        assert!(
            !current_after(&|dir, _, _| {
                fs::remove_file(dir.join("pack").join(INDEX_NAME)).unwrap();
            }),
            "missing index"
        );
    }

    /// One bake with one job, which writes `content` and dates it `age` ago,
    /// then the pack step. Returns whether the pack was written.
    fn bake_and_pack(dir: &Path, content: &str, age: Duration) -> bool {
        let manifest = dir.join("manifest.yaml");
        if !manifest.exists() {
            fs::write(&manifest, "").unwrap();
            set_time(&manifest, SystemTime::now() - HOUR);
        }
        let mut baker = Baker::new(&manifest, &dir.join("root")).unwrap();
        let job = Job::new("a", vec![], move |to| {
            fs::write(to, content)?;
            set_time(to, SystemTime::now() - age);
            Ok(())
        });
        baker.run(vec![job]).unwrap();
        update(&baker, &dir.join("pack"), CAP).unwrap().is_some()
    }

    fn pack_times(dir: &Path) -> Vec<SystemTime> {
        [INDEX_NAME, "assets_000.spndat"]
            .map(|name| {
                let path = dir.join("pack").join(name);
                fs::metadata(path).unwrap().modified().unwrap()
            })
            .to_vec()
    }

    #[test]
    fn a_job_that_dates_its_output_in_the_past_writes_the_pack_again() {
        let dir = tempfile::tempdir().unwrap();
        // As the bake does for a font with a stale saved atlas.
        let age = HOUR * 24 * 365 * 10;
        assert!(bake_and_pack(dir.path(), "one", age));
        assert!(bake_and_pack(dir.path(), "two", age));
        // The dates and sizes alone do not show the change.
        assert!(is_current(
            &dir.path().join("root"),
            &["a".to_string()],
            &dir.path().join("pack"),
            &[]
        ));
        check_content(dir.path());
    }

    #[test]
    fn a_bake_with_no_job_to_run_does_not_touch_the_pack() {
        let dir = tempfile::tempdir().unwrap();
        assert!(bake_and_pack(dir.path(), "one", Duration::ZERO));
        // A date that a new write cannot give.
        for name in [INDEX_NAME, "assets_000.spndat"] {
            set_time(
                &dir.path().join("pack").join(name),
                SystemTime::now() + HOUR,
            );
        }
        let before = pack_times(dir.path());
        assert!(!bake_and_pack(dir.path(), "never runs", Duration::ZERO));
        assert_eq!(pack_times(dir.path()), before);
    }

    #[test]
    fn the_pack_folder_cannot_be_in_the_output_folder() {
        let dir = tempfile::tempdir().unwrap();
        let output = dir.path().join("out");
        for bad in ["out", "out/pack", "new/../out/pack"] {
            assert!(
                check_folder(&dir.path().join(bad), &output).is_err(),
                "{bad}"
            );
        }
        for good in ["pack", "out-pack", "out/../pack"] {
            check_folder(&dir.path().join(good), &output).unwrap();
        }
    }
}
