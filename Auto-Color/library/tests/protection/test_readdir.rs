use libc::{opendir, readdir, closedir};
use std::ffi::CString;

#[test]
fn test_readdir_block() {
    let path = CString::new("/etc").unwrap();
    let dir = unsafe { opendir(path.as_ptr()) };
    assert!(!dir.is_null(), "Expected opendir to succeed for /etc");

    let mut found = false;
    loop {
        let entry = unsafe { readdir(dir) };
        if entry.is_null() {
            break;
        }

        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        if entry_name == "ld.so.preload" {
            found = true;
            break;
        }
    }

    assert!(!found, "Expected readdir to not return ld.so.preload in /etc");

    unsafe { closedir(dir) };
}

#[test]
fn test_readdir_allow() {
    let path = CString::new("/etc").unwrap();
    let dir = unsafe { opendir(path.as_ptr()) };

    let entry = unsafe { readdir(dir) };
    assert!(!entry.is_null(), "Expected readdir to return a valid entry for /etc");

    unsafe { closedir(dir) };
}

#[test]
fn test_readdir64_block() {
    let path = CString::new("/etc").unwrap();
    let dir = unsafe { libc::opendir(path.as_ptr()) };
    assert!(!dir.is_null(), "Expected opendir to succeed for /etc");

    let mut found = false;
    loop {
        let entry = unsafe { libc::readdir64(dir) };
        if entry.is_null() {
            break;
        }

        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        if entry_name == "ld.so.preload" {
            found = true;
            break;
        }
    }

    assert!(!found, "Expected readdir64 to not return ld.so.preload in /etc");

    unsafe { libc::closedir(dir) };
}

#[test]
fn test_readdir64_allow() {
    let path = CString::new("/etc").unwrap();
    let dir = unsafe { opendir(path.as_ptr()) };

    let entry = unsafe { readdir64(dir) };
    assert!(!entry.is_null(), "Expected readdir64 to return a valid entry for /etc");

    unsafe { closedir(dir) };
}