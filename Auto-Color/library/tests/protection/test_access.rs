use libc::{access, F_OK};
use std::ffi::CString;

#[test]
fn test_access_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let result = unsafe { access(path.as_ptr(), F_OK) };
    assert_eq!(result, -1, "Expected access to fail for /etc/ld.so.preload");
}

#[test]
fn test_access_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let result = unsafe { access(path.as_ptr(), F_OK) };
    assert_eq!(result, 0, "Expected access to succeed for /etc/passwd");
}
