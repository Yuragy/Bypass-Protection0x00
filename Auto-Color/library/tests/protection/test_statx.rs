use libc::{statx, AT_FDCWD, STATX_BASIC_STATS};
use std::ffi::CString;
use std::mem::MaybeUninit;

#[test]
fn test_statx_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let mut statx_buf = MaybeUninit::uninit();
    let result = unsafe { statx(AT_FDCWD, path.as_ptr(), 0, STATX_BASIC_STATS, statx_buf.as_mut_ptr()) };
    assert_eq!(result, -1, "Expected statx to fail for /etc/ld.so.preload");
}

#[test]
fn test_statx_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let mut statx_buf = MaybeUninit::uninit();
    let result = unsafe { statx(AT_FDCWD, path.as_ptr(), 0, STATX_BASIC_STATS, statx_buf.as_mut_ptr()) };
    assert_eq!(result, 0, "Expected statx to succeed for /etc/passwd");
}
