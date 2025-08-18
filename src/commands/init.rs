use std::{env, path::PathBuf};

use crate::GlobalContext;
use crate::commands::CmdResult;
use anyhow::anyhow;

pub fn exec(ctx: &mut GlobalContext, parser: &mut lexopt::Parser) -> CmdResult {
    use lexopt::prelude::*;

    let mut template = PathBuf::new();

    while let Some(arg) = parser.next()? {
        match arg {
            Value(s) => {
                let path = PathBuf::from(s);
                if path.is_absolute() {
                    return Err(anyhow!("paths to templates cannot be absolute"));
                }
                template.push(path);
            }
            _ => return Err(anyhow!(arg.unexpected())),
        }
    }

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

    let proj = env::current_dir()?;
    if proj.read_dir()?.next().is_some() {
        return Err(anyhow!("project directory isnt empty"));
    }

    super::new::do_init(&dir, &proj)?;

    Ok(())
}
