use std::ffi::{OsStr, OsString};
use std::fs;
use std::path::{Path, PathBuf};

use crate::GlobalContext;
use crate::commands::CmdResult;
use crate::util;
use anyhow::anyhow;
use walkdir::WalkDir;

// cao get

pub fn exec(ctx: &mut GlobalContext, parser: &mut lexopt::Parser) -> CmdResult {
    use lexopt::prelude::*;

    let mut template = PathBuf::new();
    let mut projmaybe: Option<OsString> = None;

    while let Some(arg) = parser.next()? {
        match arg {
            Value(s) => {
                if projmaybe.is_some() {
                    let path = PathBuf::from(s);
                    if path.is_absolute() {
                        return Err(anyhow!("paths to templates cannot be absolute"));
                    }
                    template.push(path);
                } else {
                    projmaybe = Some(s);
                }
            }
            _ => return Err(anyhow!(arg.unexpected())),
        }
    }

    let projname = projmaybe.ok_or(anyhow!("project name not set"))?;
    if template.as_os_str().is_empty() {
        return Err(anyhow!("cannot use empty template"));
    }

    let mut dir = ctx
        .proj_dirs
        .config_dir()
        .to_owned()
        .join("templates")
        .join(template.clone());

    if dir.exists() && dir.is_dir() {
        let new_dir = dir.join(template.clone());
        if new_dir.exists() {
            dir = new_dir;
        }
    } else {
        return Err(anyhow!("template '{}' does not exist", dir.display()));
    }

    let proj = PathBuf::from(projname);
    if proj.exists() && proj.read_dir()?.next().is_some() {
        return Err(anyhow!("project directory isnt empty"));
    }

    do_init(&dir, &proj)?;

    Ok(())
}

pub fn do_init(from: &Path, to: &Path) -> CmdResult {
    fs::create_dir_all(to)?;
    let projname = to
        .file_name()
        .unwrap_or(OsStr::new("project_name"))
        .to_string_lossy()
        .to_string();

    for entry in WalkDir::new(from).into_iter().filter_map(|e| e.ok()) {
        let from_path = entry.path();
        let relative_path = from_path.strip_prefix(from)?;

        let to_path = to.join(relative_path);

        if entry.file_type().is_dir() {
            fs::create_dir_all(to_path)?;

            if !relative_path.as_os_str().is_empty() {
                eprintln!("cao: created {}", relative_path.display());
            }
        } else if entry.file_type().is_file() {
            if let Some(parent) = to_path.parent() {
                fs::create_dir_all(parent)?;
            }
            if let Err(e) =
                util::copy_and_replace(from_path, &to_path, "{{PROJECTNAME}}", &projname)
            {
                eprintln!("cao: {} {}", e, from_path.display());
            } else {
                eprintln!("cao: created {}", relative_path.display());
            }
        }
    }

    Ok(())
}
