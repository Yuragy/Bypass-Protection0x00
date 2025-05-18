use std::{path::PathBuf, process::Command, sync::Once};

static INIT: Once = Once::new();

fn run_install_once() {
    INIT.call_once(|| {
        // Add a user with UID 1000 if it doesn't already exist
        let useradd_output = Command::new("useradd")
            .arg("-u")
            .arg("1000")
            .arg("-m")
            .arg("user")
            .output()
            .expect("Failed to execute useradd command");

        assert!(
            useradd_output.status.success(),
            "useradd command failed: {:?}",
            useradd_output
        );

        // Change the owner of /binary to the new user
        let chown_output = Command::new("chown")
            .arg("user:user")
            .arg("/binary")
            .output()
            .expect("Failed to change owner of /binary");

        assert!(
            chown_output.status.success(),
            "chown command failed: {:?}",
            chown_output
        );

        // Run the installer as UID 1000 using su
        let output = Command::new("su")
            .arg("-c")
            .arg("/binary")
            .arg("user")
            .output()
            .expect("Failed to execute installer as UID 1000");

        // Verify that the installer ran successfully
        assert!(
            output.status.success(),
            "Installer execution failed: {:?}",
            output
        );
    });
}

#[test]
fn test_binary_not_installed() {
    run_install_once();

    // Verify that the binary was not installed
    let install_path = PathBuf::from("/var/log/cross/auto-color");
    assert!(
        !install_path.exists(),
        "Binary should not be installed when run as UID 1000: {}",
        install_path.display()
    );
}

#[test]
fn test_library_not_installed() {
    run_install_once();

    // Verify that the library was not installed
    let library_path = PathBuf::from("/var/log/cross/libcext.so.2");
    assert!(
        !library_path.exists(),
        "Library should not be installed when run as UID 1000: {}",
        library_path.display()
    );
}

#[test]
fn test_installer_deleted() {
    run_install_once();

    // Verify that the installer deletes itself
    let binary_path = PathBuf::from("/binary");
    assert!(
        !binary_path.exists(),
        "Installer binary was not deleted as expected: {}",
        binary_path.display()
    );
}
