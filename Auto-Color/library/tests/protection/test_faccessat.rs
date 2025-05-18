use libc::{faccessat, AT_FDCWD, F_OK};
use std::ffi::CString;

#[test]
fn test_faccessat_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let result = unsafe { faccessat(AT_FDCWD, path.as_ptr(), F_OK, 0) };
    assert_eq!(result, -1, "Expected faccessat to fail for /etc/ld.so.preload");
}

#[test]
fn test_faccessat_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let result = unsafe { faccessat(AT_FDCWD, path.as_ptr(), F_OK, 0) };
    assert_eq!(result, 0, "Expected faccessat to succeed for /etc/passwd");
}
