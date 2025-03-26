use crate::GlobalContext;

pub mod help;
pub mod init;
pub mod new;
pub mod version;

pub type CmdResult = anyhow::Result<()>;
pub type Exec = fn(&mut GlobalContext, &mut lexopt::Parser) -> CmdResult;

pub fn find_command(s: &str) -> Option<Exec> {
    let func = match s {
        "help" => help::exec,
        "init" => init::exec,
        "new" => new::exec,
        "version" => version::exec,
        _ => return None,
    };
    Some(func)
}
