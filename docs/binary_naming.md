# Binary Naming Convention

Lumos uses different binary names for development and release builds to avoid confusion and allow both to coexist.

## Binary Names

### Development Build: `lumos_dev`
- **Location**: `build/src/applications/lumos_simple/lumos_dev`
- **When**: Built with `LUMOS_OFFICIAL_RELEASE=OFF` (default)
- **Purpose**: Local development, testing, and debugging
- **LUMOS_ROOT detection**: Walks up from executable to find project root with `src/` structure

### Release Build: `lumos`
- **Location**: `release/lumos-macos-1.0.0/bin/lumos` → installed to `/usr/local/bin/lumos`
- **When**: Built with `LUMOS_OFFICIAL_RELEASE=ON` (via `create_macos_release.py`)
- **Purpose**: Distribution and end-user installation
- **LUMOS_ROOT detection**: Looks for flattened structure at `../share/lumos`

## Building

### Development Build
```bash
# Default build (creates lumos_dev)
mkdir -p build && cd build
cmake ..
cmake --build .

# Binary created at: build/src/applications/lumos_simple/lumos_dev
```

Output:
```
-- Building DEVELOPMENT version
-- Building development binary: lumos_dev
```

### Release Build
```bash
# Official release build (creates lumos)
mkdir -p build && cd build
cmake -DLUMOS_OFFICIAL_RELEASE=ON ..
cmake --build .

# Binary created at: build/src/applications/lumos_simple/lumos
```

Output:
```
-- Building OFFICIAL RELEASE
-- Building release binary: lumos
```

### Automated Release Build
```bash
# Uses create_macos_release.py (automatically sets LUMOS_OFFICIAL_RELEASE=ON)
python3 scripts/create_macos_release.py
```

## Usage

### Development
```bash
# Direct execution
./build/src/applications/lumos_simple/lumos_dev build

# Via wrapper script (with tab completion support)
./lumos_cli build
source ./lumos_cli  # Enable tab completion
```

### Release (Installed)
```bash
# After installation via Homebrew or install.sh
lumos build
lumos flash
lumos monitor
```

## Why Different Names?

**Benefits:**
1. **Clear distinction**: Immediately know if running dev or release version
2. **Coexistence**: Both binaries can exist simultaneously without conflicts
3. **Safety**: Prevents accidentally distributing debug builds
4. **Testing**: Can test release builds locally before distribution

**Example Scenario:**
```bash
# Development workflow
./lumos_cli build              # Uses lumos_dev

# Test release build locally
brew install --build-from-source ./homebrew-tools/Formula/lumos.rb
lumos build                    # Uses installed release lumos

# Both coexist peacefully!
```

## Implementation

The binary name is set in `src/applications/lumos_simple/CMakeLists.txt`:

```cmake
if(LUMOS_OFFICIAL_RELEASE)
    set_target_properties(lumos_target PROPERTIES OUTPUT_NAME "lumos")
else()
    set_target_properties(lumos_target PROPERTIES OUTPUT_NAME "lumos_dev")
endif()
```

## Verification

Check which binary you're running:

```bash
# Development
$ ./build/src/applications/lumos_simple/lumos_dev --version
Lumos v1.0.0

# Release
$ lumos --version
Lumos v1.0.0
```

Both display the same version, but the binary names differ for clarity.
