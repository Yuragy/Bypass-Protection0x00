use libc::{open, read, close, O_RDONLY};
use library::with_hook_protection;
use std::ffi::CString;

#[test]
fn test_read_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();

    // Use with_hook_protection to allow opening the file
    let fd = with_hook_protection(
        || Some(unsafe { open(path.as_ptr(), O_RDONLY) }),
        || unsafe { open(path.as_ptr(), O_RDONLY) },
    );

    // Attempt to read from the file descriptor outside of hook protection
    let mut buffer = [0u8; 128];
    let bytes_read = unsafe { read(fd, buffer.as_mut_ptr() as *mut _, buffer.len()) };
    assert_eq!(bytes_read, -1, "Expected read to fail for /etc/ld.so.preload outside of hook protection");

    unsafe { close(fd) };
}

#[test]
fn test_read_allow() {
    let path = CString::new("/etc/passwd").unwrap();
    let fd = unsafe { open(path.as_ptr(), O_RDONLY) };
    assert!(fd >= 0, "Expected open to succeed for /etc/passwd");

    let mut buffer = [0u8; 128];
    let bytes_read = unsafe { read(fd, buffer.as_mut_ptr() as *mut _, buffer.len()) };
    assert!(bytes_read > 0, "Expected read to succeed for /etc/passwd");

    unsafe { close(fd) };
}
