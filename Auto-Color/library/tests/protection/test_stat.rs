use libc::stat;
use std::ffi::CString;
use std::mem::MaybeUninit;

#[test]
fn test_stat_block() {
    let path = CString::new("/etc/ld.so.preload").unwrap();
    let mut stat_buf = MaybeUninit::uninit();
    let result = unsafe { stat(path.as_ptr(), stat_buf.as_mut_ptr()) };
    assert_eq!(result, -1, "Expected stat to fail for /etc/ld.so.preload");
}

#[test]
fn test_stat_allow() {
    // TODO: Stat issues
    // let path = CString::new("/etc/passwd").unwrap();
    // let mut stat_buf = MaybeUninit::uninit();
    // let result = unsafe { stat(path.as_ptr(), stat_buf.as_mut_ptr()) };
    // assert_eq!(result, 0, "Expected stat to succeed for /etc/passwd");
}
