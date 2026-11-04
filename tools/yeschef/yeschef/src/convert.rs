//! The converters. Each one is a stub until its stage lands. The manifest and
//! bake code already calls them, so a stage replaces a body and nothing else.

use anyhow::Result;
use std::fmt;
use std::path::Path;

/// A converter that does not exist yet. A bake that fails with this keeps the
/// output it found, because nothing was converted.
#[derive(Debug)]
pub struct NotImplemented(pub &'static str);

impl fmt::Display for NotImplemented {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} is not implemented yet", self.0)
    }
}

impl std::error::Error for NotImplemented {}

pub fn model(_from: &Path, _to: &Path) -> Result<()> {
    Err(NotImplemented("model conversion").into())
}

pub fn verify(_source: &Path, _baked: &Path) -> Result<()> {
    Err(NotImplemented("verify").into())
}
