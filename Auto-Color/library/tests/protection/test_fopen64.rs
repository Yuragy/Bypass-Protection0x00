use libc::{fopen64, fclose};
use std::ffi::CString;

#[test]
fn test_fopen64_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let file = unsafe { fopen64(path.as_ptr(), CString::new("r").unwrap().as_ptr()) };
    assert!(file.is_null(), "Expected fopen64 to fail for /etc/ld.so.preload");
}

#[test]
fn test_fopen64_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let file = unsafe { fopen64(path.as_ptr(), CString::new("r").unwrap().as_ptr()) };
    assert!(!file.is_null(), "Expected fopen64 to succeed for /etc/passwd");
    unsafe { fclose(file) };
}
