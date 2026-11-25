use std::collections::BTreeMap;
use std::fs;
use std::path::Path;
use std::process::{Command, Output};
use yeschef_format::font::Tables;
use yeschef_format::ktx2;

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
fn a_font_atlas_extracts_and_inserts() {
    let atlas = |side: u32, texel: u8| {
        let level = vec![texel; (side * side * 3) as usize];
        ktx2::write(ktx2::Format::R8G8B8Unorm, side, side, &[level], &[])
    };
    let tables = Tables {
        source_hash: 0,
        slot_count: 0,
        plain: BTreeMap::new(),
        tabular: BTreeMap::new(),
        sizes: Vec::new(),
    };
    let dir = tempfile::tempdir().unwrap();
    let font = dir.path().join("font.spnga");
    let edit = dir.path().join("atlas.ktx2");
    let original = tables.to_file(&atlas(4, 1));
    fs::write(&font, &original).unwrap();

    let run = yeschef(&["--extract-atlas", path(&font), path(&edit)]);
    assert!(run.status.success(), "{run:?}");
    assert_eq!(fs::read(&edit).unwrap(), atlas(4, 1));

    for bad in [atlas(8, 2), b"not a KTX2 file".to_vec()] {
        fs::write(&edit, bad).unwrap();
        let run = yeschef(&["--insert-atlas", path(&edit), path(&font)]);
        assert_eq!(run.status.code(), Some(1), "{run:?}");
        assert_eq!(fs::read(&font).unwrap(), original);
    }

    fs::write(&edit, atlas(4, 2)).unwrap();
    let run = yeschef(&["--insert-atlas", path(&edit), path(&font)]);
    assert!(run.status.success(), "{run:?}");
    assert_eq!(fs::read(&font).unwrap(), tables.to_file(&atlas(4, 2)));
}

#[test]
fn a_saved_atlas_is_baked_until_the_layout_changes() {
    let dir = tempfile::tempdir().unwrap();
    let inter = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../assets/fonts/inter.ttf");
    fs::copy(inter, dir.path().join("inter.ttf")).unwrap();
    let manifest = dir.path().join("manifest.yaml");
    let out = dir.path().join("out");
    let font = out.join("inter.spnga");
    let saved = dir.path().join("saved.ktx2");
    let bake_with = |sizes: &str, atlas: &str, flags: &[&str]| {
        let text = format!(
            "fonts:\n  - source: inter.ttf\n    output: inter.spnga\n    sizes: {sizes}\n{atlas}"
        );
        fs::write(&manifest, text).unwrap();
        let mut args = vec!["--manifest", path(&manifest), path(&out)];
        args.extend(flags);
        yeschef(&args)
    };
    let bake = |sizes: &str, atlas: &str| bake_with(sizes, atlas, &[]);
    let entry =
        |layout: &str| format!("    atlas:\n      file: saved.ktx2\n      layout: \"{layout}\"\n");

    let run = bake("[12]", "");
    assert!(run.status.success(), "{run:?}");
    let run = yeschef(&["--extract-atlas", path(&font), path(&saved)]);
    assert!(run.status.success(), "{run:?}");
    let stdout = String::from_utf8_lossy(&run.stdout);
    let layout = stdout.trim().strip_prefix("layout ").unwrap().to_string();

    // Edit one texel of the saved atlas.
    let mut edited = fs::read(&saved).unwrap();
    let last = edited.len() - 1;
    edited[last] ^= 0xFF;
    fs::write(&saved, &edited).unwrap();

    let run = bake("[12]", &entry(&layout));
    assert!(run.status.success(), "{run:?}");
    assert!(fs::read(&font).unwrap().ends_with(&edited));

    for (sizes, id) in [("[12, 14]", layout.as_str()), ("[12]", "0000000000000000")] {
        let run = bake(sizes, &entry(id));
        assert_eq!(run.status.code(), Some(1), "{run:?}");
        assert!(String::from_utf8_lossy(&run.stderr).contains("different layout"));
        assert!(!font.exists(), "no font with a stale or a drawn atlas");
    }

    // The debug flag uses the drawn atlas and warns on every run.
    let flags = ["--allow-stale-atlas"];
    for _ in 0..2 {
        let run = bake_with("[12, 14]", &entry(&layout), &flags);
        assert!(run.status.success(), "{run:?}");
        let stdout = String::from_utf8_lossy(&run.stdout);
        assert!(
            stdout.contains("warning: using the drawn atlas"),
            "{stdout}"
        );
        assert!(!fs::read(&font).unwrap().ends_with(&edited));
    }
    // A strict run in the same folder does not accept that font as current.
    // The manifest keeps its time, so only the age of the font forces a bake.
    let run = yeschef(&["--manifest", path(&manifest), path(&out)]);
    assert_eq!(run.status.code(), Some(1), "{run:?}");

    // A broken or missing atlas fails with the flag too.
    fs::write(&saved, "not a KTX2 file").unwrap();
    let run = bake_with("[12, 14]", &entry(&layout), &flags);
    assert_eq!(run.status.code(), Some(1), "{run:?}");
    fs::remove_file(&saved).unwrap();
    let run = bake_with("[12, 14]", &entry(&layout), &flags);
    assert_eq!(run.status.code(), Some(1), "{run:?}");
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
fn a_pack_goes_outside_the_output_folder() {
    let dir = tempfile::tempdir().unwrap();
    let manifest = dir.path().join("manifest.yaml");
    fs::write(&manifest, "licenses: []\n").unwrap();
    let out = dir.path().join("out");
    let bake = |pack: &Path| {
        yeschef(&[
            "--manifest",
            path(&manifest),
            path(&out),
            "--pack",
            path(pack),
        ])
    };

    for inside in [out.clone(), out.join("pack")] {
        let run = bake(&inside);
        assert_eq!(run.status.code(), Some(2), "{run:?}");
        assert!(!inside.join("assets.spnidx").exists());
    }

    let pack = dir.path().join("pack");
    let run = bake(&pack);
    assert!(run.status.success(), "{run:?}");
    assert!(pack.join("assets.spnidx").exists());
}

#[test]
fn a_failed_conversion_removes_its_output_and_skips_the_sweep() {
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
    assert!(String::from_utf8_lossy(&run.stderr).contains("unable to read"));
    assert!(!out.join("a.spnga").exists(), "a stale output is not kept");
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
fn help_starts_with_the_name_and_version() {
    let run = yeschef(&["--help"]);
    assert_eq!(run.status.code(), Some(0));
    let help = String::from_utf8(run.stdout).unwrap();
    assert!(help.starts_with(concat!("yeschef ", env!("CARGO_PKG_VERSION"), "\n")));
}

#[test]
fn usage_errors_exit_2() {
    assert_eq!(yeschef(&[]).status.code(), Some(2));
}

#[test]
fn verify_of_missing_files_fails() {
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
        let run = Command::new(env!("CARGO_BIN_EXE_yeschef"))
            .env("RAYON_NUM_THREADS", threads)
            .args(["--manifest", path(&manifest), path(&out)])
            .output()
            .unwrap();
        assert_eq!(run.status.code(), Some(1));
        String::from_utf8(run.stderr).unwrap()
    };
    let one = stderr("1");
    assert_eq!(one, stderr("8"));
    let first = one.find("m00.spnga").unwrap();
    assert!(first < one.find("m11.spnga").unwrap());
    assert!(one.contains("12 of the assets failed"));
}

#[test]
fn shaders_are_compiled_into_a_readable_pack() {
    let dir = tempfile::tempdir().unwrap();
    let source = dir.path().join("shaders");
    fs::create_dir_all(&source).unwrap();
    fs::write(
        source.join("t.slang"),
        "[shader(\"compute\")]\n[numthreads(1, 1, 1)]\nvoid main(uint3 id : SV_DispatchThreadID) {}\n",
    )
    .unwrap();
    let manifest = dir.path().join("manifest.yaml");
    fs::write(
        &manifest,
        "shaders:\n  output: shaders.spnga\n  stages:\n    t.comp: shaders/t.slang:main\n",
    )
    .unwrap();
    let out = dir.path().join("out");

    let run = yeschef(&[
        "--manifest",
        path(&manifest),
        path(&out),
        "--no-line-directives",
    ]);
    assert!(run.status.success(), "{run:?}");
    assert!(String::from_utf8_lossy(&run.stdout).contains("1 shaders ->"));

    let pack =
        yeschef_format::shaderpack::read(&fs::read(out.join("shaders.spnga")).unwrap()).unwrap();
    let glsl = &pack["t.comp"];
    assert!(glsl.starts_with("#version 450"), "{glsl}");
    assert!(!glsl.contains("#line"));

    // A missing entry point fails the bake and leaves no output.
    fs::write(
        &manifest,
        "shaders:\n  output: shaders.spnga\n  stages:\n    t.comp: shaders/t.slang:nothing\n",
    )
    .unwrap();
    let run = yeschef(&["--manifest", path(&manifest), path(&out)]);
    assert_eq!(run.status.code(), Some(1));
    assert!(String::from_utf8_lossy(&run.stderr).contains("nothing"));
    assert!(!out.join("shaders.spnga").exists());
}
