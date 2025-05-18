use libc::dirent;
use std::ffi::CString;

#[test]
fn test_scandir_block() {
    let path = CString::new("/etc").unwrap();
    let mut namelist: *mut *mut dirent = std::ptr::null_mut();

    // Attempt to scan the directory
    let result = unsafe { library::protect_scandir(path.as_ptr(), &mut namelist, None, None) };
    assert!(result >= 0, "Expected scandir to succeed for /etc");

    // Ensure ld.so.preload is not visible
    let mut found = false;
    for i in 0..result {
        let entry = unsafe { *namelist.add(i as usize) };
        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        if entry_name == "ld.so.preload" {
            found = true;
            break;
        }
    }
    assert!(!found, "Expected scandir to not return ld.so.preload in /etc");

    // Free the allocated memory for namelist
    if !namelist.is_null() {
        for i in 0..result {
            unsafe {
                libc::free(*namelist.add(i as usize) as *mut libc::c_void);
            }
        }
        unsafe {
            libc::free(namelist as *mut libc::c_void);
        }
    }
}

#[test]
fn test_scandir_allow() {
    let path = CString::new("/etc").unwrap();
    let mut namelist: *mut *mut dirent = std::ptr::null_mut();

    // Attempt to scan the directory
    let result = unsafe { library::protect_scandir(path.as_ptr(), &mut namelist, None, None) };
    assert!(result >= 0, "Expected scandir to succeed for /etc");

    // Check that a valid dirent is returned
    assert!(!namelist.is_null(), "Expected namelist to be non-null");
    let first_entry = unsafe { *namelist };
    assert!(!first_entry.is_null(), "Expected a valid dirent to be returned");

    // Free the allocated memory for namelist
    if !namelist.is_null() {
        for i in 0..result {
            unsafe {
                libc::free(*namelist.add(i as usize) as *mut libc::c_void);
            }
        }
        unsafe {
            libc::free(namelist as *mut libc::c_void);
        }
    }
}

#[test]
fn test_scandir_filter() {
    let path = CString::new("/etc").unwrap();
    let mut namelist: *mut *mut dirent = std::ptr::null_mut();

    // Define a filter function to exclude entries containing "w"
    unsafe extern "C" fn filter(entry: *const dirent) -> libc::c_int {
        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr())
            .to_string_lossy()
            .to_string() };
        (!entry_name.contains('w')) as libc::c_int
    }

    // Attempt to scan the directory with the filter
    let result = unsafe { library::protect_scandir(path.as_ptr(), &mut namelist, Some(filter), None) };
    assert!(result >= 0, "Expected scandir to succeed for /etc");

    // Ensure a valid entry is returned and neither "passwd" nor "ld.so.preload" are included
    let mut found_passwd = false;
    let mut found_ld_so_preload = false;
    for i in 0..result {
        let entry = unsafe { *namelist.add(i as usize) };
        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        if entry_name == "passwd" {
            found_passwd = true;
        }
        if entry_name == "ld.so.preload" {
            found_ld_so_preload = true;
        }
    }
    assert!(!found_passwd, "Expected scandir to not return passwd in /etc");
    assert!(!found_ld_so_preload, "Expected scandir to not return ld.so.preload in /etc");

    // Free the allocated memory for namelist
    if !namelist.is_null() {
        for i in 0..result {
            unsafe {
                libc::free(*namelist.add(i as usize) as *mut libc::c_void);
            }
        }
        unsafe {
            libc::free(namelist as *mut libc::c_void);
        }
    }
}

#[test]
fn test_scandir64_block() {
    let path = CString::new("/etc").unwrap();
    let mut namelist: *mut *mut libc::dirent64 = std::ptr::null_mut();

    let result = unsafe { library::protect_scandir64(path.as_ptr(), &mut namelist, None, None) };
    assert!(result >= 0, "Expected scandir64 to succeed for /etc");

    let mut found = false;
    for i in 0..result {
        let entry = unsafe { *namelist.add(i as usize) };
        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        if entry_name == "ld.so.preload" {
            found = true;
            break;
        }
    }
    assert!(!found, "Expected scandir64 to not return ld.so.preload in /etc");

    if !namelist.is_null() {
        for i in 0..result {
            unsafe {
                libc::free(*namelist.add(i as usize) as *mut libc::c_void);
            }
        }
        unsafe {
            libc::free(namelist as *mut libc::c_void);
        }
    }
}

#[test]
fn test_scandir64_allow() {
    let path = CString::new("/etc").unwrap();
    let mut namelist: *mut *mut libc::dirent64 = std::ptr::null_mut();

    // Attempt to scan the directory
    let result = unsafe { library::protect_scandir64(path.as_ptr(), &mut namelist, None, None) };
    assert!(result >= 0, "Expected scandir64 to succeed for /etc");

    // Check that a valid dirent64 is returned
    assert!(!namelist.is_null(), "Expected namelist to be non-null");
    let first_entry = unsafe { *namelist };
    assert!(!first_entry.is_null(), "Expected a valid dirent64 to be returned");

    // Free the allocated memory for namelist
    if !namelist.is_null() {
        for i in 0..result {
            unsafe {
                libc::free(*namelist.add(i as usize) as *mut libc::c_void);
            }
        }
        unsafe {
            libc::free(namelist as *mut libc::c_void);
        }
    }
}

#[test]
fn test_scandir64_filter() {
    let path = CString::new("/etc").unwrap();
    let mut namelist: *mut *mut libc::dirent64 = std::ptr::null_mut();

    // Define a filter function to exclude entries containing "w"
    unsafe extern "C" fn filter(entry: *const libc::dirent64) -> libc::c_int {
        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        (!entry_name.contains('w')) as libc::c_int
    }

    // Attempt to scan the directory with the filter
    let result = unsafe { library::protect_scandir64(path.as_ptr(), &mut namelist, Some(filter), None) };
    assert!(result >= 0, "Expected scandir64 to succeed for /etc");

    // Ensure a valid entry is returned and neither "passwd" nor "ld.so.preload" are included
    let mut found_passwd = false;
    let mut found_ld_so_preload = false;
    for i in 0..result {
        let entry = unsafe { *namelist.add(i as usize) };
        let entry_name = unsafe { std::ffi::CStr::from_ptr((*entry).d_name.as_ptr()) }
            .to_string_lossy()
            .to_string();
        if entry_name == "passwd" {
            found_passwd = true;
        }
        if entry_name == "ld.so.preload" {
            found_ld_so_preload = true;
        }
    }
    assert!(!found_passwd, "Expected scandir64 to not return passwd in /etc");
    assert!(!found_ld_so_preload, "Expected scandir64 to not return ld.so.preload in /etc");

    // Free the allocated memory for namelist
    if !namelist.is_null() {
        for i in 0..result {
            unsafe {
                libc::free(*namelist.add(i as usize) as *mut libc::c_void);
            }
        }
        unsafe {
            libc::free(namelist as *mut libc::c_void);
        }
    }
}
