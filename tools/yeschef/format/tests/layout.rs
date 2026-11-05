use std::fs;
use std::path::{Path, PathBuf};

fn header_path() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../../sponge/src/scene/format/assetlayout.generated.hpp")
}

fn generate() -> String {
    let dir = env!("CARGO_MANIFEST_DIR");
    let config = cbindgen::Config::from_file(Path::new(dir).join("cbindgen.toml")).unwrap();
    let mut out = Vec::new();
    cbindgen::Builder::new()
        .with_crate(dir)
        .with_config(config)
        .generate()
        .expect("cbindgen failed")
        .write(&mut out);
    String::from_utf8(out).unwrap().replace("\r\n", "\n")
}

/// The checked-in C++ header must be what cbindgen makes from the Rust source.
/// Run with `UPDATE_LAYOUT_HEADER=1` to rewrite it.
#[test]
fn header_is_current() {
    let want = generate();
    let path = header_path();
    if std::env::var_os("UPDATE_LAYOUT_HEADER").is_some() {
        fs::write(&path, &want).unwrap();
        return;
    }
    let have = fs::read_to_string(&path)
        .unwrap_or_default()
        .replace("\r\n", "\n");
    let first_difference = have
        .lines()
        .zip(want.lines())
        .enumerate()
        .find(|(_, (a, b))| a != b)
        .map(|(n, (a, b))| format!("line {}: checked in `{a}`, generated `{b}`", n + 1))
        .unwrap_or_else(|| {
            format!(
                "{} lines checked in, {} generated",
                have.lines().count(),
                want.lines().count()
            )
        });
    assert!(
        have == want,
        "{} is stale ({first_difference}). Run `UPDATE_LAYOUT_HEADER=1 cargo test` in tools/yeschef.",
        path.display()
    );
}
