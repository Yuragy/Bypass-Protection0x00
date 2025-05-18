use std::{fs, path::PathBuf, process::Command, sync::Once};
use libc::{dladdr, strerror, Dl_info};
use std::{ffi::CStr, os::raw::c_void};

static INIT: Once = Once::new();

fn run_install_once() {
    INIT.call_once(|| {
        // Run the binary to trigger the installation process
        assert!(Command::new("/binary").output().unwrap().status.success());
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
fn test_binary_installs_itself() {
    run_install_once();

    // Verify that the binary was copied to the expected location
    let install_path = PathBuf::from("/var/log/cross/auto-color");
    assert!(
        install_path.exists(),
        "Binary was not installed at the expected location: {}",
        install_path.display()
    );
}

#[test]
fn test_library_exists_after_install() {
    run_install_once();

    // Dynamically find the library path
    let base_path = find_library_path();
    let library_path = base_path.join("libcext.so.2");

    // Verify that the library exists at the expected location
    assert!(
        library_path.exists(),
        "Library does not exist at the expected location: {}",
        library_path.display()
    );
}

#[test]
fn test_installer_removed() {
    run_install_once();

    // Verify that the installer binary deletes itself
    let binary_path = PathBuf::from("/binary");
    assert!(
        !binary_path.exists(),
        "Installer binary was not deleted as expected: {}",
        binary_path.display()
    );
}

#[test]
fn test_ld_preload_file() {
    run_install_once();

    // Verify that the `LD_PRELOAD` file contains the expected library path
    let base_path = find_library_path();
    let library_path = base_path.join("libcext.so.2");
    let preload_path = PathBuf::from("/etc/ld.so.preload");

    assert!(
        preload_path.exists(),
        "LD_PRELOAD file does not exist at the expected location: {}",
        preload_path.display()
    );

    let preload_contents = fs::read_to_string(&preload_path).expect("Failed to read LD_PRELOAD file");
    assert!(
        preload_contents.contains(library_path.to_string_lossy().as_ref()),
        "LD_PRELOAD file does not contain the expected library path: {}",
        library_path.display()
    );
}

#[test]
fn test_ld_preload_cache() {
    run_install_once();

    // Run `ldd` on the binary to check for the libc extension
    let output = Command::new("ldd")
        .arg("/bin/ls")
        .output()
        .expect("Failed to execute `ldd` command");

    let combined_output = format!(
        "{}{}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );

    assert!(
        combined_output.contains("libcext.so.2"),
        "Expected `ldd` output to contain 'libcext.so.2', but got: {}",
        combined_output
    );
}

#[test]
fn test_auto_color_log_output() {
    run_install_once();

    // Set the AUTO_COLOR_LOG environment variable and attempt to `cat` the `/etc/ld.so.preload` file
    let output = Command::new("cat")
        .arg("/etc/ld.so.preload")
        .env("AUTO_COLOR_LOG", "info")
        .output()
        .expect("Failed to execute `cat` command");

    let combined_output = format!(
        "{}{}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );

    // Check that the log lines indicate the file opening was prevented
    assert!(
        combined_output.contains("[library]"),
        "Expected log lines from Auto-Color in the output, but none were found."
    );
    assert!(
        combined_output.contains("Prevented opening of /etc/ld.so.preload"),
        "Expected log line indicating `/etc/ld.so.preload` opening was prevented, but none were found."
    );
}
