# Ensure the script is run as root
if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root. Please re-run with sudo or as root."
    exit 1
fi

# Remove the copied executable
rm -rf /var/log/cross

# Remove the preload hook entry
if [ -f /etc/ld.so.preload ]; then
    sed -i 's|[^;[:space:]]*libcext.so.2[;[:space:]]*||g' /etc/ld.so.preload
    echo "Removed libcext.so.2 entry from /etc/ld.so.preload"
else
    echo "/etc/ld.so.preload not found."
fi

# Find and remove the libcext.so.2 file
LIBRARY_PATH=$(find /lib -name "libcext.so.2" 2>/dev/null)
if [ -n "$LIBRARY_PATH" ]; then
    echo "Removing library at $LIBRARY_PATH"
    rm -f "$LIBRARY_PATH"
else
    echo "Library libcext.so.2 not found."
fi

