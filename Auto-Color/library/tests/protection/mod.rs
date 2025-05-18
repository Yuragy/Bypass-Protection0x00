// TODO: Without compiling another package, we cannot test the _xstat functions
// This should be fine as they call the underlying stat functions

mod test_open;
mod test_open64;
mod test_openat;
mod test_openat64;
mod test_fopen;
mod test_fopen64;
mod test_rename;
mod test_access;
mod test_realpath;
mod test_read;
mod test_pread;
mod test_chmod;
mod test_fchmodat;
mod test_fchmod;
mod test_unlink;
mod test_unlinkat;
mod test_renameat;
mod test_stat;
mod test_statx;
mod test_lstat;
mod test_fstat;
mod test_fstatat;
mod test_faccessat;
mod test_opendir;
mod test_readdir;
mod test_scandir;
