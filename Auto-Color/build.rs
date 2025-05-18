use std::fs;
use std::path::Path;
use std::process::Command;

fn main() {
    // Watch for changes in the "library/src" directory
    let library_src_dir = Path::new("library/src");
    for entry in fs::read_dir(library_src_dir).unwrap() {
        let path = entry.unwrap().path();
        if path.is_file() {
            println!("cargo:rerun-if-changed=library/src/{}", path.file_name().unwrap().to_string_lossy());
        }
    }

    // Run `cargo build` for the "library" sub-crate
    let status = Command::new("cargo")
        .arg("build")
        .arg("--release")
        .current_dir("library") // Set the working directory to the "library" sub-crate
        .status()
        .expect("Failed to execute cargo build for library sub-crate");

    if !status.success() {
        panic!("cargo build for library sub-crate failed");
    }
    println!("hello");
}