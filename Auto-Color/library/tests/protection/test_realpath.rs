use libc::realpath;
use std::ffi::CString;

#[test]
fn test_realpath_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let mut resolved_path = [0 as libc::c_char; libc::PATH_MAX as usize];
    let result = unsafe { realpath(path.as_ptr(), resolved_path.as_mut_ptr()) };
    assert!(result.is_null(), "Expected realpath to fail for /etc/ld.so.preload");
}

#[test]
fn test_realpath_allow() {
    let path = CString::new("/etc/../etc/passwd").unwrap();
    let mut resolved_path = [0 as libc::c_char; libc::PATH_MAX as usize];
    let result = unsafe { realpath(path.as_ptr(), resolved_path.as_mut_ptr()) };
    assert!(!result.is_null(), "Expected realpath to succeed for /etc/../etc/passwd");
}
