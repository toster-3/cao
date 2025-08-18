use std::process::Command;

use anyhow::anyhow;

use crate::GlobalContext;
use crate::commands::build::BuildType;
use crate::commands::{CmdResult, build};
use crate::util::Show;

pub fn exec(ctx: &mut GlobalContext, parser: &mut lexopt::Parser) -> CmdResult {
    let b = build::BuildType::from_dir(&ctx.cwd);
    if b == BuildType::Cargo {
        Command::new("cargo")
            .args(["run", "--"])
            .args(
                parser
                    .try_raw_args()
                    .ok_or_else(|| anyhow!("it broke ig"))?,
            )
            .show()
            .status()?;
    }

    ctx.projname = ctx
        .cwd
        .file_name()
        .ok_or_else(|| anyhow!("it broke ig"))?
        .into();
    let proj_exe = &ctx.cwd.join("build").join(&ctx.projname);
    build::do_build(ctx, b)?;

    Command::new(proj_exe)
        .args(
            parser
                .try_raw_args()
                .ok_or_else(|| anyhow!("it broke ig"))?,
        )
        .show()
        .status()?;
    Ok(())
}
