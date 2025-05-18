use libc::rename;
use std::ffi::CString;

#[test]
fn test_rename_block() {
    let old_path = CString::new("/etc/ld.so.preload").unwrap();
    let new_path = CString::new("/tmp/newfile").unwrap();
    let result = unsafe { rename(old_path.as_ptr(), new_path.as_ptr()) };
    assert_eq!(result, -1, "Expected rename to fail for /etc/ld.so.preload");
}

#[test]
fn test_rename_allow() {
    let old_path = CString::new("/tmp/testfile").unwrap();
    let new_path = CString::new("/tmp/newfile").unwrap();

    // Create a file beforehand
    let fd = unsafe { libc::open(old_path.as_ptr(), libc::O_CREAT | libc::O_WRONLY, 0o644) };
    assert!(fd >= 0, "Expected file creation to succeed for /tmp/testfile");
    unsafe { libc::close(fd) };

    // Attempt to rename the file
    let result = unsafe { rename(old_path.as_ptr(), new_path.as_ptr()) };
    assert_eq!(result, 0, "Expected rename to succeed for /tmp/testfile");

    // Remove the renamed file
    let unlink_result = unsafe { libc::unlink(new_path.as_ptr()) };
    assert_eq!(unlink_result, 0, "Expected unlink to succeed for /tmp/newfile");
}
