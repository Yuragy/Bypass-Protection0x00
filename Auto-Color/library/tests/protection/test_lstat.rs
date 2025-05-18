use libc::lstat;
use std::ffi::CString;
use std::mem::MaybeUninit;

#[test]
fn test_lstat_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let mut stat_buf = MaybeUninit::uninit();
    let result = unsafe { lstat(path.as_ptr(), stat_buf.as_mut_ptr()) };
    assert_eq!(result, -1, "Expected lstat to fail for /etc/ld.so.preload");
}

#[test]
fn test_lstat_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let mut stat_buf = MaybeUninit::uninit();
    let result = unsafe { lstat(path.as_ptr(), stat_buf.as_mut_ptr()) };
    assert_eq!(result, 0, "Expected lstat to succeed for /etc/passwd");

    // Ensure the returned stat buffer is valid (size > 0)
    let stat_buf = unsafe { stat_buf.assume_init() };
    assert!(stat_buf.st_size > 0, "Expected st_size to be greater than 0 for /etc/passwd");
}
