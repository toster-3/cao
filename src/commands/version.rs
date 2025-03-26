use crate::GlobalContext;
use crate::commands::CmdResult;

pub fn exec(_: &mut GlobalContext, _: &mut lexopt::Parser) -> CmdResult {
    eprintln!("cao 0.1.0");
    Ok(())
}
