use std::{fs, path::PathBuf, process::Command, sync::Once};
use libc::{dladdr, strerror, Dl_info};
use std::{ffi::CStr, os::raw::c_void};

static INIT: Once = Once::new();

fn run_install_once() {
    INIT.call_once(|| {
        // Run the binary to trigger the installation process
        assert!(Command::new("/binary").output().unwrap().status.success());

        // Run the `env` command with `LD_PRELOAD` set to force-load the library
        Command::new("env")
            .env("AUTO_DESATURATE", "1") 
            .env("AUTO_COLOR_LOG_LEVEL", "INFO") 
            //.env("LD_PRELOAD", library_path.to_string_lossy().as_ref()) // Force-load the library
            .output()
            .expect("Failed to execute `env` command");

    });
}

fn find_library_path() -> PathBuf {
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
            return path_buf;
        }
    }
    PathBuf::from("/lib")
}

#[test]
fn test_no_binary_artifact() {
    run_install_once();

    // Verify that the auto-color binary artifact is not present
    let install_path = "/var/log/cross/auto-color";
    assert!(
        !fs::metadata(install_path).is_ok(),
        "Installer artifact found at: {}",
        install_path
    );
}

#[test]
fn test_no_library_artifact() {
    run_install_once();

    // Verify that the library artifact is not present
    let library_path = find_library_path().join("libcext.so.2");
    assert!(
        !fs::metadata(&library_path).is_ok(),
        "Installer artifact found at: {}",
        library_path.display()
    );
}

#[test]
fn test_ld_preload_does_not_contain_library() {
    run_install_once();

    // Verify that the `LD_PRELOAD` file does not contain the library path
    let preload_path = "/etc/ld.so.preload";
    let library_path = find_library_path().join("libcext.so.2");

    if let Ok(preload_contents) = fs::read_to_string(preload_path) {
        assert!(
            !preload_contents.contains(library_path.to_string_lossy().as_ref()),
            "LD_PRELOAD file still contains the library path after uninstall: {}",
            library_path.display()
        );
    } else {
        // If the file does not exist, it is also a valid state after uninstall
        assert!(
            !PathBuf::from(preload_path).exists(),
            "LD_PRELOAD file unexpectedly exists after uninstall."
        );
    }
}
