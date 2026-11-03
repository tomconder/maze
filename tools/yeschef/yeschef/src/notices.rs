//! The third-party notices file.

use anyhow::{Context, Result, bail};
use std::collections::BTreeMap;
use std::fs;
use std::path::{Path, PathBuf};

/// License name to license files, sorted by name.
pub type Licenses = BTreeMap<String, Vec<PathBuf>>;

/// One notices file with a section per license. A section with several files
/// lists each under its file name, as `vcpkg_install_copyright` does.
///
/// The file is rewritten only when its text changes, because the list comes
/// partly from the command line, which has no timestamp. Returns the byte count if it
/// wrote.
pub fn write_notices(output: &Path, licenses: &Licenses) -> Result<Option<usize>> {
    let rule = "=".repeat(80);
    let mut text: Vec<u8> = Vec::new();
    for (name, files) in licenses {
        text.extend_from_slice(format!("{rule}\n{name}\n{rule}\n\n").as_bytes());
        for path in files {
            let mut contents =
                fs::read(path).with_context(|| format!("unable to read {}", path.display()))?;
            if contents.is_empty() {
                bail!("unable to read {}", path.display());
            }
            contents.retain(|&byte| byte != b'\r');
            if files.len() > 1 {
                let file_name = path.file_name().unwrap_or_default().to_string_lossy();
                text.extend_from_slice(format!("{file_name}:\n\n").as_bytes());
            }
            text.extend_from_slice(&contents);
            text.extend_from_slice(b"\n\n");
        }
    }

    if fs::read(output).is_ok_and(|current| current == text) {
        return Ok(None);
    }
    if let Some(parent) = output.parent() {
        fs::create_dir_all(parent)
            .with_context(|| format!("unable to create {}", parent.display()))?;
    }
    fs::write(output, &text).with_context(|| format!("unable to write {}", output.display()))?;
    Ok(Some(text.len()))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn writes_one_section_per_license() {
        let dir = tempfile::tempdir().unwrap();
        let single = dir.path().join("single.txt");
        let first = dir.path().join("first.txt");
        let second = dir.path().join("second.txt");
        fs::write(&single, "MIT text\r\n").unwrap();
        fs::write(&first, "one").unwrap();
        fs::write(&second, "two").unwrap();

        let mut licenses = Licenses::new();
        licenses.insert("Zeta".into(), vec![single]);
        licenses.insert("Alpha".into(), vec![first, second]);
        let output = dir.path().join("out/NOTICES.txt");
        assert!(write_notices(&output, &licenses).unwrap().is_some());

        let rule = "=".repeat(80);
        let want = format!(
            "{rule}\nAlpha\n{rule}\n\nfirst.txt:\n\none\n\nsecond.txt:\n\ntwo\n\n\
             {rule}\nZeta\n{rule}\n\nMIT text\n\n\n"
        );
        assert_eq!(fs::read_to_string(&output).unwrap(), want);
    }

    #[test]
    fn leaves_an_unchanged_file_alone() {
        let dir = tempfile::tempdir().unwrap();
        let license = dir.path().join("l.txt");
        fs::write(&license, "text").unwrap();
        let mut licenses = Licenses::new();
        licenses.insert("A".into(), vec![license]);
        let output = dir.path().join("NOTICES.txt");
        assert!(write_notices(&output, &licenses).unwrap().is_some());
        assert!(write_notices(&output, &licenses).unwrap().is_none());
    }

    #[test]
    fn a_missing_or_empty_license_fails() {
        let dir = tempfile::tempdir().unwrap();
        let empty = dir.path().join("empty.txt");
        fs::write(&empty, "").unwrap();
        for path in [empty, dir.path().join("missing.txt")] {
            let mut licenses = Licenses::new();
            licenses.insert("A".into(), vec![path]);
            assert!(write_notices(&dir.path().join("N.txt"), &licenses).is_err());
        }
    }
}
