use std::fs;
use std::path::Path;
use std::process::{Command, Output};

fn yeschef(args: &[&str]) -> Output {
    Command::new(env!("CARGO_BIN_EXE_yeschef"))
        .args(args)
        .output()
        .unwrap()
}

fn path(p: &Path) -> &str {
    p.to_str().unwrap()
}

#[test]
fn licenses_only_manifest_writes_notices_and_removes_orphans() {
    let dir = tempfile::tempdir().unwrap();
    fs::write(dir.path().join("lic.txt"), "text\r\n").unwrap();
    let manifest = dir.path().join("manifest.yaml");
    fs::write(&manifest, "licenses:\n  - name: A\n    files: [lic.txt]\n").unwrap();
    let out = dir.path().join("out");
    fs::create_dir_all(&out).unwrap();
    fs::write(out.join("old.bin"), "x").unwrap();
    let notices = dir.path().join("NOTICES.txt");

    let run = yeschef(&[
        "--manifest",
        path(&manifest),
        path(&out),
        "--notices",
        path(&notices),
    ]);
    assert!(run.status.success(), "{run:?}");
    let stdout = String::from_utf8_lossy(&run.stdout);
    assert!(stdout.contains("removed orphan"), "{stdout}");
    assert!(stdout.contains("1 licenses ->"), "{stdout}");
    assert!(!out.join("old.bin").exists());
    let rule = "=".repeat(80);
    assert_eq!(
        fs::read_to_string(&notices).unwrap(),
        format!("{rule}\nA\n{rule}\n\ntext\n\n\n")
    );

    let again = yeschef(&[
        "--manifest",
        path(&manifest),
        path(&out),
        "--notices",
        path(&notices),
    ]);
    assert!(again.status.success());
    assert!(again.stdout.is_empty(), "unchanged notices stay quiet");
}

#[test]
fn a_missing_converter_fails_and_keeps_existing_outputs() {
    let dir = tempfile::tempdir().unwrap();
    let manifest = dir.path().join("manifest.yaml");
    fs::write(
        &manifest,
        "models:\n  - source: a.glb\n    output: a.spnga\n",
    )
    .unwrap();
    let out = dir.path().join("out");
    fs::create_dir_all(&out).unwrap();
    fs::write(out.join("a.spnga"), "baked").unwrap();
    fs::write(out.join("other.bin"), "x").unwrap();

    let run = yeschef(&["--manifest", path(&manifest), path(&out)]);
    assert_eq!(run.status.code(), Some(1));
    assert!(String::from_utf8_lossy(&run.stderr).contains("not implemented"));
    assert_eq!(fs::read_to_string(out.join("a.spnga")).unwrap(), "baked");
    assert!(out.join("other.bin").exists(), "no sweep after a failure");
}

#[test]
fn a_bad_manifest_exits_2() {
    let dir = tempfile::tempdir().unwrap();
    let manifest = dir.path().join("manifest.yaml");
    fs::write(&manifest, "model: []\n").unwrap();
    let run = yeschef(&["--manifest", path(&manifest), path(dir.path())]);
    assert_eq!(run.status.code(), Some(2));
}

#[test]
fn usage_errors_exit_2() {
    assert_eq!(yeschef(&[]).status.code(), Some(2));
    let run = yeschef(&["--threads", "0", "--manifest", "m", "o"]);
    assert_eq!(run.status.code(), Some(2));
}

#[test]
fn verify_is_not_implemented() {
    assert_eq!(yeschef(&["--verify", "a", "b"]).status.code(), Some(1));
}

#[test]
fn errors_come_in_manifest_order_for_any_thread_count() {
    let dir = tempfile::tempdir().unwrap();
    let manifest = dir.path().join("manifest.yaml");
    let models: String = (0..12)
        .map(|i| format!("  - source: {i}.glb\n    output: m{i:02}.spnga\n"))
        .collect();
    fs::write(&manifest, format!("models:\n{models}")).unwrap();
    let out = dir.path().join("out");

    let stderr = |threads: &str| {
        let run = yeschef(&[
            "--threads",
            threads,
            "--manifest",
            path(&manifest),
            path(&out),
        ]);
        assert_eq!(run.status.code(), Some(1));
        String::from_utf8(run.stderr).unwrap()
    };
    let one = stderr("1");
    assert_eq!(one, stderr("8"));
    let first = one.find("m00.spnga").unwrap();
    assert!(first < one.find("m11.spnga").unwrap());
    assert!(one.contains("12 of the assets failed"));
}
