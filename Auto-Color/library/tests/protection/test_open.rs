use libc::{c_char, O_RDONLY};
use std::ffi::CString;

#[test]
fn test_open_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let fd = unsafe { libc::open(path.as_ptr() as *const c_char, O_RDONLY) };
    assert_eq!(fd, -1, "Expected open to fail for /etc/ld.so.preload");
}

#[test]
fn test_open_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let fd = unsafe { libc::open(path.as_ptr() as *const c_char, O_RDONLY) };
    assert!(fd >= 0, "Expected open to succeed for /etc/passwd");
    unsafe { libc::close(fd) }; 
}
