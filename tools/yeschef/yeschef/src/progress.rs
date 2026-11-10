//! A progress display on stderr in the style of `docker pull`: one line per
//! asset, a spinner while it bakes and a tick when it is done, above a bar
//! that counts the assets. It draws only on a terminal. A pipe
//! (the CMake build) gets the converters' plain stdout lines, as before.

use console::style;
use indicatif::{MultiProgress, ProgressBar, ProgressStyle};
use std::collections::HashMap;
use std::io::IsTerminal;
use std::path::{Path, PathBuf};
use std::sync::Mutex;
use std::time::Duration;

/// The asset and output folders, which `display` strips from a path.
static ROOTS: Mutex<Vec<PathBuf>> = Mutex::new(Vec::new());
/// Statistics lines held back while the terminal display is drawn.
static STATS: Mutex<Vec<String>> = Mutex::new(Vec::new());
static DISPLAY: Mutex<Option<MultiProgress>> = Mutex::new(None);

/// Reports a converter's statistics line. A plain run prints it at once. The
/// terminal display holds the lines and prints them as a summary at the end.
macro_rules! say {
    ($($arg:tt)*) => { $crate::progress::report(&format!($($arg)*)) };
}
pub(crate) use say;

/// A path relative to the asset or output folder, with forward slashes.
pub fn display(path: &Path) -> String {
    let roots = ROOTS.lock().unwrap();
    let short = roots
        .iter()
        .find_map(|root| path.strip_prefix(root).ok())
        .unwrap_or(path);
    short.to_string_lossy().replace('\\', "/")
}

pub fn report(line: &str) {
    if DISPLAY.lock().unwrap().is_some() {
        STATS.lock().unwrap().push(line.to_string());
    } else {
        println!("{line}");
    }
}

struct Terminal {
    display: MultiProgress,
    total: ProgressBar,
    lines: Mutex<HashMap<String, ProgressBar>>,
}

pub struct Progress(Option<Terminal>);

fn template(text: &str) -> ProgressStyle {
    ProgressStyle::with_template(text).expect("valid template")
}

impl Progress {
    pub fn new(total: usize, roots: Vec<PathBuf>) -> Self {
        *ROOTS.lock().unwrap() = roots;
        Self(std::io::stderr().is_terminal().then(|| {
            let display = MultiProgress::new();
            let total = display.add(
                ProgressBar::new(total as u64)
                    .with_style(
                        template("{prefix:>12.cyan.bold} [{bar:27}] {pos}/{len}")
                            .progress_chars("=> "),
                    )
                    .with_prefix("Baking"),
            );
            // Draw the bar now. A bar that first draws later shifts the lines below it.
            total.tick();
            *DISPLAY.lock().unwrap() = Some(display.clone());
            Terminal {
                display,
                total,
                lines: Mutex::new(HashMap::new()),
            }
        }))
    }

    pub fn start(&self, name: &str) {
        if let Some(t) = &self.0 {
            let line = t.display.add(
                ProgressBar::new_spinner()
                    .with_style(template("{spinner:.cyan} {msg} {elapsed:.dim}"))
                    .with_message(name.to_string()),
            );
            line.enable_steady_tick(Duration::from_millis(80));
            t.lines.lock().unwrap().insert(name.to_string(), line);
        }
    }

    /// An asset that needed no bake.
    pub fn skip(&self) {
        if let Some(t) = &self.0 {
            t.total.inc(1);
        }
    }

    pub fn done(&self, name: &str) {
        self.finish(name, style("✔").green());
    }

    pub fn failed(&self, name: &str) {
        self.finish(name, style("✘").red());
    }

    /// Leaves the asset's line on screen with a mark and its bake time.
    fn finish(&self, name: &str, mark: console::StyledObject<&str>) {
        if let Some(t) = &self.0 {
            if let Some(line) = t.lines.lock().unwrap().remove(name) {
                line.set_style(template("{msg}"));
                let took = line.elapsed().as_secs_f32();
                line.finish_with_message(format!(
                    "{mark} {name} {}",
                    style(format!("{took:.1}s")).dim()
                ));
            }
            t.total.inc(1);
        }
    }
}

impl Drop for Progress {
    fn drop(&mut self) {
        if let Some(t) = &self.0 {
            t.total.finish();
            *DISPLAY.lock().unwrap() = None;
            let mut stats = std::mem::take(&mut *STATS.lock().unwrap());
            stats.sort();
            for line in stats {
                println!("{line}");
            }
        }
    }
}
