use libc::{fchmod, open, close, O_RDONLY};
use std::ffi::CString;
use std::mem::MaybeUninit;

#[test]
fn test_fchmod_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();

    // Use with_hook_protection to allow opening the file
    let fd = library::with_hook_protection(
        || Some(unsafe { open(path.as_ptr(), O_RDONLY) }),
        || unsafe { open(path.as_ptr(), O_RDONLY) },
    );
    assert!(fd >= 0, "Expected open to succeed for /etc/ld.so.preload within hook protection");

    // Attempt to change file permissions
    let result = unsafe { fchmod(fd, 0o600) };
    assert_eq!(result, -1, "Expected fchmod to fail for /etc/ld.so.preload");

    unsafe { close(fd) };
}

#[test]
fn test_fchmod_allow() {
    let path = CString::new("/tmp/testfile").unwrap();

    // Create a test file
    let fd = unsafe { libc::open(path.as_ptr(), libc::O_CREAT | libc::O_WRONLY, 0o644) };
    assert!(fd >= 0, "Expected file creation to succeed for /tmp/testfile");

    // Attempt to change file permissions
    let result = unsafe { fchmod(fd, 0o600) };
    assert_eq!(result, 0, "Expected fchmod to succeed for /tmp/testfile");

    // Verify the file permissions TODO: Stat issues
    // let mut stat_buf = MaybeUninit::uninit();
    // let stat_result = unsafe { libc::fstat(fd, stat_buf.as_mut_ptr()) };
    // assert_eq!(stat_result, 0, "Expected fstat to succeed for /tmp/testfile");
    // let stat_buf = unsafe { stat_buf.assume_init() };
    // assert_eq!(stat_buf.st_mode & 0o777, 0o600, "Expected file permissions to be 0o600 for /tmp/testfile");

    unsafe { libc::close(fd) };

    // Remove the test file
    let unlink_result = unsafe { libc::unlink(path.as_ptr()) };
    assert_eq!(unlink_result, 0, "Expected unlink to succeed for /tmp/testfile");
}
