pub mod build;
pub mod help;
pub mod init;
pub mod new;
pub mod run;
pub mod version;

pub type CmdResult = anyhow::Result<()>;
pub type Exec = fn(&mut crate::GlobalContext, &mut lexopt::Parser) -> CmdResult;

pub fn find_command(s: &str) -> Option<Exec> {
    let func = match s {
        "build" => build::exec,
        "help" => help::exec,
        "init" => init::exec,
        "new" => new::exec,
        "run" => run::exec,
        "version" => version::exec,
        _ => return None,
    };
    Some(func)
}
