use std::{env, path::PathBuf, process};

use directories::ProjectDirs;

mod commands;
mod util;

struct GlobalContext {
    cwd: PathBuf,
    proj_dirs: ProjectDirs,
}

impl GlobalContext {
    fn new(cwd: PathBuf) -> GlobalContext {
        let proj_dirs = directories::ProjectDirs::from("fart", "toster-3", "cao")
            .expect("project dirs kant be found ??? try using a sane os.");

        GlobalContext { cwd, proj_dirs }
    }

    fn update_cwd(&mut self) -> anyhow::Result<()> {
        let cwd = env::current_dir()?;

        self.cwd = cwd;
        Ok(())
    }
}

fn main() -> anyhow::Result<()> {
    use lexopt::prelude::*;

    let cwd = std::env::current_dir()?;
    let mut ctx = GlobalContext::new(cwd);

    let mut parser = lexopt::Parser::from_env();
    while let Some(arg) = parser.next()? {
        match arg {
            Short('V') | Long("version") => {
                commands::version::exec(&mut ctx, &mut parser)?;
                process::exit(0);
            }
            Short('C') | Long("directory") => {
                env::set_current_dir(parser.value()?)?;
                ctx.update_cwd()?;
            }
            Value(os) => {
                let s = os.to_str().expect("Invalid UTF-8 in command");
                if let Some(func) = commands::find_command(s) {
                    func(&mut ctx, &mut parser)?;
                }
            }
            _ => {}
        }
    }

    Ok(())
}
