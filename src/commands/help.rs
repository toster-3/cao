use crate::GlobalContext;
use crate::commands::CmdResult;

pub fn exec(_: &mut GlobalContext, _: &mut lexopt::Parser) -> CmdResult {
    eprintln!("cao help msg");
    Ok(())
}
