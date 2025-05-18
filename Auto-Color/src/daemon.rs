use std::{ffi::CString, os::unix::prelude::RawFd};
use libc::{
    _exit, chdir, close, dup2, flock, fork, geteuid, open, setsid, umask, LOCK_EX, LOCK_NB, O_CREAT, O_RDWR, S_IRUSR, S_IWUSR
};
use log::{info, error, warn};

pub fn main() {
    info!("🚀 Starting daemon process...");
    if check_lock() {
        fork_to_bg();
        redirect_to_dev_null();
        close_all_fds();
        unsafe {
            umask(0);
            if chdir(CString::new("/").unwrap().as_ptr()) != 0 {
                error!("⚠️ Failed to change directory to root.");
            }
        }
        info!("✅ Daemon process initialized successfully.");
        loop {
            info!("🔄 Daemon is running...");
            std::thread::sleep(std::time::Duration::from_secs(60));
        }
    } else {
        error!("⚠️ Another instance is already running.");
    }
}

fn fork_to_bg() {
    info!("🔧 Forking to background...");
    unsafe {
        if fork() > 0 {
            _exit(0);
        }
        if setsid() < 0 {
            error!("⚠️ Failed to create a new session.");
            _exit(1);
        }
        if fork() > 0 {
            _exit(0);
        }
    }
    info!("✅ Forked to background successfully.");
}

fn redirect_to_dev_null() {
    info!("🔄 Redirecting input, output, and error to /dev/null...");
    let dev_null = CString::new("/dev/null").unwrap();
    unsafe {
        let fd = open(dev_null.as_ptr(), O_RDWR, 0);
        if fd >= 0 {
            dup2(fd, libc::STDIN_FILENO);
            dup2(fd, libc::STDOUT_FILENO);
            dup2(fd, libc::STDERR_FILENO);
            close(fd);
            info!("✅ Redirection to /dev/null completed.");
        } else {
            error!("⚠️ Failed to open /dev/null.");
        }
    }
}

fn close_all_fds() {
    info!("🔒 Closing all file descriptors...");
    unsafe {
        let max_fd = libc::sysconf(libc::_SC_OPEN_MAX);
        if max_fd > 0 {
            for fd in 3..(max_fd as i32) {
                close(fd);
            }
            info!("✅ All file descriptors closed.");
        } else {
            error!("⚠️ Failed to retrieve the maximum number of file descriptors.");
        }
    }
}

pub fn check_lock() -> bool {
    info!("🔍 Checking for lock file...");
    let euid = unsafe { geteuid() };
    let Ok(lock_path) = CString::new(format!("/tmp/config-err-17EF88CF{}", euid)) else {
        error!("⚠️ Failed to create lock file path.");
        return false;
    };

    unsafe {
        let fd: RawFd = open(lock_path.as_ptr(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if fd < 0 {
            error!("⚠️ Failed to open lock file.");
            return false;
        }

        let result = flock(fd, LOCK_EX | LOCK_NB);
        if result == 0 {
            info!("✅ Lock acquired successfully.");
            true
        } else {
            warn!("⚠️ Failed to acquire lock. Another instance might be running.");
            false
        }
    }
}