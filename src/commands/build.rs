use std::ffi::{OsStr, OsString};
use std::path::Path;
use std::process::Command;
use std::{env, fs};

use anyhow::anyhow;

use crate::GlobalContext;
use crate::commands::CmdResult;
use crate::util::Show;

pub fn exec(ctx: &mut GlobalContext, _: &mut lexopt::Parser) -> CmdResult {
    let b = BuildType::from_dir(&ctx.cwd);
    do_build(ctx, b)
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum BuildType {
    Cargo,
    Make,
    Cmake,
    Other,
}

#[allow(unused)]
impl BuildType {
    pub fn from_str(s: &OsStr) -> Option<BuildType> {
        use BuildType as B;

        let b = match s.to_str()? {
            "Cargo.toml" => B::Cargo,
            "Makefile" | "makefile" => B::Make,
            "CMakeLists.txt" => B::Cmake,
            _ => return None,
        };
        Some(b)
    }

    pub fn from_dir(p: &Path) -> BuildType {
        if p.join("Cargo.toml").exists() {
            BuildType::Cargo
        } else if p.join("Makefile").exists() || p.join("makefile").exists() {
            BuildType::Make
        } else if p.join("CMakeLists.txt").exists() {
            BuildType::Cmake
        } else {
            BuildType::Other
        }
    }

    pub fn from_env() -> anyhow::Result<BuildType> {
        Ok(BuildType::from_dir(&env::current_dir()?))
    }

    pub fn choose(self, other: BuildType) -> BuildType {
        if self < other { self } else { other }
    }

    pub fn into_vec(self) -> Option<Vec<OsString>> {
        use BuildType as B;
        use OsString as O;
        let ret = match self {
            B::Cargo => vec![O::from("cargo"), O::from("build")],
            B::Make => vec![O::from("make")],
            // NOTE: maybe do cmake later ?
            B::Cmake => do_cmake()?,
            _ => return None,
        };
        Some(ret)
    }
}

// NOTE: discarding all errors might not be the best idea ?
fn do_cmake() -> Option<Vec<OsString>> {
    use OsString as O;
    let cur_dir = env::current_dir().ok()?;
    let build_dir = cur_dir.join("build");
    if build_dir.exists() {
        let b = BuildType::from_dir(&build_dir);
        if b == BuildType::Make {
            Some(vec![O::from("make"), O::from("-Cbuild")])
        } else {
            Some(vec![O::from("ninja"), O::from("-Cbuild")])
        }
    } else {
        // FIXME: cant check if ninja or make is used
        Some(vec![O::from("cmake"), O::from("-Bbuild")])
    }
}

pub fn do_build(ctx: &GlobalContext, b: BuildType) -> anyhow::Result<()> {
    if b != BuildType::Cmake {
        let v = b.into_vec().ok_or(anyhow!("No build !"))?;
        if let Some((cmd, args)) = v.split_first() {
            Command::new(cmd).args(args).show().status()?;
        } else {
            return Err(anyhow!("No build !"));
        }
    } else {
        fn thing(p: &Path) -> anyhow::Result<()> {
            if p.join("Makefile").exists() || p.join("makefile").exists() {
                Command::new("make").arg("-Cbuild").status()?;
            } else {
                Command::new("ninja").arg("-Cbuild").status()?;
            }
            Ok(())
        }

        let build_dir = env::current_dir()?.join("build");
        if build_dir.exists() {
            thing(&build_dir)?;
        } else {
            fs::create_dir(&build_dir)?;
            thing(&build_dir)?;
        }
    }

    Ok(())
}
