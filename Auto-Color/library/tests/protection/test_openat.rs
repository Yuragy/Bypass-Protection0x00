use libc::{openat, AT_FDCWD, O_RDONLY};
use std::ffi::CString;

#[test]
fn test_openat_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let fd = unsafe { openat(AT_FDCWD, path.as_ptr(), O_RDONLY) };
    assert_eq!(fd, -1, "Expected openat to fail for /etc/ld.so.preload");
}

#[test]
fn test_openat_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let fd = unsafe { openat(AT_FDCWD, path.as_ptr(), O_RDONLY) };
    assert!(fd >= 0, "Expected openat to succeed for /etc/passwd");
    unsafe { libc::close(fd) };
}
