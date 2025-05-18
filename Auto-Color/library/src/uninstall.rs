use std::path::PathBuf;
use std::ffi::CStr;
use std::fs;
use std::os::raw::c_void;
use libc::{Dl_info, dladdr};
use crate::with_hook_protection;
use regex::Regex;

pub fn uninstall() {
    with_hook_protection(|| {
        remove_copied_executable();
        remove_preload_hook();
        remove_library_file();
        Some(())
    }, ||{});
}

fn remove_copied_executable() {
    let cross_path = PathBuf::from("/var/log/cross");
    if cross_path.exists() {
        fs::remove_dir_all(&cross_path).unwrap_or_else(|err| {
            eprintln!("Failed to remove {}: {}", cross_path.display(), err);
        });
    }
}

fn remove_preload_hook() {
    let preload_path = PathBuf::from("/etc/ld.so.preload");
    if preload_path.exists() {
        let content = fs::read_to_string(&preload_path).unwrap_or_default();
        let regex = Regex::new(r"(?:^|;)[^;]*libcext\.so\.2").unwrap();
        let updated_content = regex.replace_all(&content, "").to_string();
        fs::write(&preload_path, updated_content).unwrap_or_else(|err| {
            eprintln!("Failed to update {}: {}", preload_path.display(), err);
        });
    }
}

fn remove_library_file() {
    let library_path = find_library_path().join("libcext.so.2");
    if library_path.exists() {
        fs::remove_file(&library_path).unwrap_or_else(|err| {
            eprintln!("Failed to remove {}: {}", library_path.display(), err);
        });
    }
}

fn find_library_path() -> PathBuf {
    let mut dl_info = Dl_info {
        dli_fname: core::ptr::null(),
        dli_fbase: core::ptr::null_mut(),
        dli_sname: core::ptr::null(),
        dli_saddr: core::ptr::null_mut(),
    };
    if unsafe { dladdr(find_library_path as *const c_void, &mut dl_info as *mut Dl_info) } != 0 && !dl_info.dli_fname.is_null() {
        if let Ok(path) = unsafe { CStr::from_ptr(dl_info.dli_fname) }.to_str() {
            let mut path_buf = PathBuf::from(path);
            path_buf.pop();
            return path_buf;
        }
    }
    PathBuf::from("/lib")
}