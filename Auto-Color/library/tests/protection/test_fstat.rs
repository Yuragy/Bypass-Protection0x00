use libc::{fstat, open, close, O_RDONLY};
use library::with_hook_protection;
use std::ffi::CString;
use std::mem::MaybeUninit;

#[test]
fn test_fstat_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();

    // Use with_hook_protection to allow opening the file
    let fd = with_hook_protection(
        || Some(unsafe { open(path.as_ptr(), O_RDONLY) }),
        || unsafe { open(path.as_ptr(), O_RDONLY) },
    );
    assert!(fd >= 0, "Expected open to succeed for /etc/ld.so.preload within hook protection");

    // Attempt to get file stats
    let mut stat_buf = MaybeUninit::uninit();
    let result = unsafe { fstat(fd, stat_buf.as_mut_ptr()) };
    assert_eq!(result, -1, "Expected fstat to fail for /etc/ld.so.preload");

    unsafe { close(fd) };
}

#[test]
fn test_fstat_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let fd = unsafe { open(path.as_ptr(), O_RDONLY) };
    assert!(fd >= 0, "Expected open to succeed for /etc/passwd");

    let mut stat_buf = MaybeUninit::uninit();
    let result = unsafe { fstat(fd, stat_buf.as_mut_ptr()) };
    assert_eq!(result, 0, "Expected fstat to succeed for /etc/passwd");

    // Ensure the returned stat buffer is valid (size > 0)
    let stat_buf = unsafe { stat_buf.assume_init() };
    assert!(stat_buf.st_size > 0, "Expected st_size to be greater than 0 for /etc/passwd");

    unsafe { close(fd) };
}
