use libc::{chmod, O_CREAT, O_WRONLY};
use std::ffi::CString;
use std::mem::MaybeUninit;

#[test]
fn test_chmod_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let result = unsafe { chmod(path.as_ptr(), 0o600) };
    assert_eq!(result, -1, "Expected chmod to fail for /etc/ld.so.preload");
}

#[test]
fn test_chmod_allow() {
    let path = CString::new("/tmp/testfile").unwrap();

    // Create a test file
    let fd = unsafe { libc::open(path.as_ptr(), O_CREAT | O_WRONLY, 0o644) };
    assert!(fd >= 0, "Expected file creation to succeed for /tmp/testfile");
    unsafe { libc::close(fd) };

    // Attempt to change file permissions
    let result = unsafe { chmod(path.as_ptr(), 0o600) };
    assert_eq!(result, 0, "Expected chmod to succeed for /tmp/testfile");

    // Verify the file permissions TODO: Stat issues
    // let mut stat_buf = MaybeUninit::uninit();
    // let stat_result = unsafe { libc::stat(path.as_ptr(), stat_buf.as_mut_ptr()) };
    // assert_eq!(stat_result, 0, "Expected stat to succeed for /tmp/testfile");
    // let stat_buf = unsafe { stat_buf.assume_init() };
    // assert_eq!(stat_buf.st_mode & 0o777, 0o600, "Expected file permissions to be 0o600 for /tmp/testfile");

    // Remove the test file
    let unlink_result = unsafe { libc::unlink(path.as_ptr()) };
    assert_eq!(unlink_result, 0, "Expected unlink to succeed for /tmp/testfile");
}
