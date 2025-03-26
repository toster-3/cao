use std::{
    fs,
    io::{Read, Write},
    path::Path,
};

use anyhow::anyhow;
use content_inspector::{ContentType, inspect};

pub fn copy_and_replace(
    from: &Path,
    to: &Path,
    pattern: &str,
    replacement: &str,
) -> anyhow::Result<()> {
    let mut file = fs::File::open(from)?;
    let mut content = String::new();
    file.read_to_string(&mut content)?;
    match inspect(content.as_bytes()) {
        ContentType::BINARY => return Ok(()),
        ContentType::UTF_8 => {}
        _ => return Err(anyhow!("non UTF-8 in 2025 ? what the frick are you doing")),
    }

    let updated = content.replace(pattern, replacement);

    if let Some(parent) = to.parent() {
        fs::create_dir_all(parent)?;
    }

    let mut file = fs::File::create(to)?;
    file.write_all(updated.as_bytes())?;

    Ok(())
}
