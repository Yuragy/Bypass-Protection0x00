use std::{process::Command, sync::Once};

static INIT: Once = Once::new();

fn run_install_once() {
    INIT.call_once(|| {
        // Run the binary to trigger the installation process
        assert!(Command::new("/binary").output().unwrap().status.success());
    });
}

#[test]
fn test_persistence_binary_running() {
    run_install_once();

    // Start `tail -f` in the background
    Command::new("tail")
        .arg("-f")
        .arg("/dev/null")
        .spawn()
        .expect("Failed to start `tail -f` process");

    // Allow some time for the persistence mechanism to trigger
    std::thread::sleep(std::time::Duration::from_secs(5));

    // Check if the symlink in `/proc/{id}/exe` points to the `auto-color` binary
    let binary_path = "/var/log/cross/auto-color";
    let mut is_running = false;

    for entry in std::fs::read_dir("/proc").expect("Failed to read /proc directory") {
        if let Ok(entry) = entry {
            let exe_path = entry.path().join("exe");
            if let Ok(target) = std::fs::read_link(&exe_path) {
                if target == std::path::Path::new(binary_path) {
                    is_running = true;
                    break;
                }
            }
    }
    }

    assert!(
        is_running,
        "Expected `auto-color` binary to be running, but it is not."
    );
}
