use libc::{fopen, fclose};
use std::ffi::CString;

#[test]
fn test_fopen_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let file = unsafe { fopen(path.as_ptr(), CString::new("r").unwrap().as_ptr()) };
    assert!(file.is_null(), "Expected fopen to fail for /etc/ld.so.preload");
}

#[test]
fn test_fopen_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let file = unsafe { fopen(path.as_ptr(), CString::new("r").unwrap().as_ptr()) };
    assert!(!file.is_null(), "Expected fopen to succeed for /etc/passwd");
    unsafe { fclose(file) };
}
