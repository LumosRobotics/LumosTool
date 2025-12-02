#!/bin/bash
#
# Cleanup script to remove unnecessary files from ARM GCC toolchain
# This reduces the toolchain size while keeping all build functionality
#
# Usage: ./cleanup_toolchain.sh <toolchain-dir-name>
# Example: ./cleanup_toolchain.sh gcc-arm-none-eabi-10.3-2021.10
#

set -e

# Check if argument is provided
if [ $# -ne 1 ]; then
    echo "Error: Toolchain directory name required"
    echo "Usage: $0 <toolchain-dir-name>"
    echo "Example: $0 gcc-arm-none-eabi-10.3-2021.10"
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TOOLCHAIN_DIR="$SCRIPT_DIR/../src/toolchains/$1"

# Check if directory exists
if [ ! -d "$TOOLCHAIN_DIR" ]; then
    echo "Error: Toolchain directory not found: $TOOLCHAIN_DIR"
    exit 1
fi

echo "=== ARM GCC Toolchain Cleanup ==="
echo "Target: $TOOLCHAIN_DIR"
echo ""
echo "Original size:"
du -sh "$TOOLCHAIN_DIR"
echo ""

# Confirm before proceeding
read -p "Proceed with cleanup? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

echo "Removing unnecessary binaries..."

# Remove debugging tools (~14 MB)
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gdb"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gdb-py"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gdb-add-index"*

# Remove profiling/coverage tools (~2 MB)
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gcov"*
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gprof"

# Remove LTO dump and analysis tools (~22 MB)
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-lto-dump"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-elfedit"

# Remove duplicate/wrapper tools (minimal size)
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gcc-10.3.1"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gcc-ar"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gcc-nm"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-gcc-ranlib"

# Remove optional analysis tools (~3 MB)
# Comment out the ones you want to keep
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-addr2line"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-c++filt"
rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-strings"
# Keep readelf - it's sometimes useful
# rm -f "$TOOLCHAIN_DIR/bin/arm-none-eabi-readelf"

echo "Removing plugin support..."
rm -rf "$TOOLCHAIN_DIR/lib/gcc/arm-none-eabi/10.3.1/plugin"

echo "Removing install tools..."
rm -rf "$TOOLCHAIN_DIR/lib/gcc/arm-none-eabi/10.3.1/install-tools"

# Optional: Remove LTO backend if you don't use -flto flag (~22 MB)
# Uncomment if you're sure you don't need LTO:
# echo "Removing LTO backend..."
# rm -f "$TOOLCHAIN_DIR/lib/gcc/arm-none-eabi/10.3.1/lto1"
# rm -f "$TOOLCHAIN_DIR/lib/gcc/arm-none-eabi/10.3.1/liblto_plugin"*

echo ""
echo "Cleanup complete!"
echo "New size:"
du -sh "$TOOLCHAIN_DIR"
echo ""
echo "Test your build with:"
echo "  cd examples/example_simple"
echo "  LUMOS_ROOT=\$(pwd)/../.. ../../build/src/applications/lumos_simple/lumos build"
echo ""
echo "If everything works, you can run this script on other toolchain versions:"
echo "  $0 <another-toolchain-dir-name>"
