# Auto-Color Malware Rewrite in Rust

This project is a rewrite of the Auto-Color malware in Rust, based on the analysis and technical details provided by [ZW01f](https://zw01f.github.io/malware%20analysis/auto-color/) and [Palo Alto's Unit 42](https://unit42.paloaltonetworks.com/new-linux-backdoor-auto-color/). Auto-Color is a Linux backdoor designed to evade detection and provide attackers with full remote access to compromised systems.

## Overview

Auto-Color disguises itself as a benign tool while employing advanced techniques to remain hidden and persist on infected systems. It hooks into libc functions to hide network activity, prevents uninstallation, and ensures its activities remain undetected.

**Note:** The Command-and-Control (C2) functionality described in the original malware is **not implemented** in this project and will **never be implemented**. This project is strictly for educational and research purposes.

### Key Features

- **Hiding Network Activity**: Hooks libc functions like `open`, `openat`, `fopen`, and `fopen64` to filter out specific entries from `/proc/net/tcp`.
- **Persistence**: Drops a malicious shared library (`libcext.so.2`) and modifies `/etc/ld.so.preload` to ensure the library is loaded in every process.
- **Evasion**: Uses benign file names and operates as a background process to avoid detection.

## Project Structure

- **`binary/`**: Contains the main executable and installation logic.
  - `src/main.rs`: Entry point for the malware.
  - `src/install.rs`: Handles installation, persistence, and library deployment.
  - `src/daemon.rs`: Implements the daemon process for background operation.
  - `build.rs`: Ensures the library is built and included during compilation.
- **`binary/library/`**: Contains the malicious shared library.
  - `src/lib.rs`: Implements hooks for libc functions to hide network activity.

## Technical Details

### Hooked Functions

The following libc functions are hooked to intercept and modify their behavior:

- **Hiding Network Activity**:
  - `open`, `open64`, `openat`, `openat64`: Intercept attempts to open `/proc/net/tcp` and return filtered data.
  - `fopen`, `fopen64`: Similar to `open`, but for higher-level file access.
- **Protecting `/etc/ld.so.preload`**:
  - `open`, `open64`, `openat`, `openat64`, `fopen`, `fopen64`: Prevents opening `/etc/ld.so.preload` for reading or writing.
  - `unlink`, `unlinkat`: Prevents deletion of `/etc/ld.so.preload`.
  - `rename`, `renameat`: Blocks renaming of `/etc/ld.so.preload`.
  - `chmod`, `fchmodat`, `fchmodat`: Restricts permission changes on `/etc/ld.so.preload`.
  - `chown`: Prevents ownership changes on `/etc/ld.so.preload`.
  - `stat`, `lstat`, `fstat`, `fstatat`, `statx`, `_lxstat`, `_fxstat`, `_xstat`: Hides metadata for `/etc/ld.so.preload`.
  - `access`, `faccessat`: Prevents access checks on `/etc/ld.so.preload`.
  - `realpath`, `getattr`: Prevents file path resolution for `/etc/ld.so.preload`.
  - `readlink`, `readlinkat`: Blocks reading symbolic links pointing to `/etc/ld.so.preload`.
  - `symlink`, `symlinkat`: Prevents creation of symbolic links targeting `/etc/ld.so.preload`.
  - `unlinkat`: Blocks unlinking of `/etc/ld.so.preload`.
  - `opendir`, `readdir`, `scandir`: Prevents directory operations that could expose `/etc/ld.so.preload`.

### Persistence Mechanism

1. Copies itself to `/var/log/cross/auto-color` with `777` permissions.
2. Drops `libcext.so.2` into the system library path.
3. Modifies `/etc/ld.so.preload` to include the malicious library, ensuring it is loaded in every process.
4. **(Planned)** Migrates to system processes (e.g., `/sbin/auditd`, `/usr/sbin/cron`) for persistence by forking and executing within these processes to avoid suspicion.

## Testing

To test the project without affecting the host operating system, a Docker-based test harness is provided. This allows the code to be executed in an isolated environment, preventing any unintended modifications or damage to the host system.

So long as Docker is installed, `cargo test` should function as usual. If Docker requires sudo, a prompt will appear. This can be validated by checking the [harness shell file](https://github.com/kcy/Auto-Color-Study/blob/main/test-harness/start-container.sh).

## References

- [ZW01f: Auto-Color Malware Analysis](https://zw01f.github.io/malware%20analysis/auto-color/)
- [Unit 42: Auto-Color - An Emerging and Evasive Linux Backdoor](https://unit42.paloaltonetworks.com/new-linux-backdoor-auto-color/)

## Disclaimer

This project is for educational and research purposes only. The use of this code for malicious purposes is strictly prohibited. The authors are not responsible for any misuse of this project.
