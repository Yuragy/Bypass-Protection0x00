
redhook::hook! {
    unsafe fn write(fd: c_int, buf: *const c_void, count: size_t) -> ssize_t => uwu_stdout {
        let orig = CStr::from_ptr(buf as *const c_char);
        if fd != 1 {
            return redhook::real!(write)(fd, buf, count);
        }
        
        let uwud = match orig.to_str() {
            Ok(txt) => uwuify_str_sse(txt),
            Err(_) => format!("Failed to uwu-ify: '{:?}'", orig),
        };
        let len = uwud.len();

        redhook::real!(write)(fd, CString::new(uwud).unwrap().as_ptr() as *const c_void, len+1)
    }
}

redhook::hook! {
    unsafe fn getuid() -> libc::uid_t => i_am_root {
        0
    }
}

// redhook::hook! {
//     unsafe fn open(path: *const c_char, oflag: c_int) -> c_int => dont_open {
//         let fd: i32 = redhook::real!(open)(path, oflag);
//         let mut file: File = File::from_raw_fd(fd);
//         let mut buf = String::new();
//         let _ = file.read_to_string(&mut buf);
//         buf = buf.to_uppercase();
    
//         let file = tmpfile();
//         fputs(CString::new(buf).unwrap().as_ptr(), file);
//         fseek(file, 0, SEEK_SET);
//         fileno(file)
//     }
// }

// V1 ------ urandom
// let new_path = "/dev/urandom\0".as_ptr() as *const c_char;
// println!("Old: {:?}, New {:?}", CStr::from_ptr(path), CStr::from_ptr(new_path));
// redhook::real!(open)(new_path, oflag)

// V2 ----- Hello World tmpfile
// let file = tmpfile();
// fputs(CString::new("Hello World\n").unwrap().as_ptr(), file);
// fseek(file, 0, SEEK_SET);
// fileno(file)

