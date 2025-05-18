use libc::{open64, O_RDONLY};
use std::ffi::CString;

#[test]
fn test_open64_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let fd = unsafe { open64(path.as_ptr(), O_RDONLY) };
    assert_eq!(fd, -1, "Expected open64 to fail for /etc/ld.so.preload");
}

#[test]
fn test_open64_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let fd = unsafe { open64(path.as_ptr(), O_RDONLY) };
    assert!(fd >= 0, "Expected open64 to succeed for /etc/passwd");
    unsafe { libc::close(fd) };
}
