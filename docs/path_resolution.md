# Lumos Path Resolution

## Overview

The `GetLumosRoot()` function determines where Lumos can find its resource files (boards, hal, platforms). It uses a priority-based approach to work in both development and distribution environments.

## Priority Order

### 1. Environment Variable (Highest Priority)
```bash
export LUMOS_ROOT=/custom/path/to/lumos
```

Users can override the automatic detection by setting `LUMOS_ROOT`.

**Use cases:**
- Testing different board configurations
- Development with multiple Lumos versions
- Custom installation locations

---

### 2. Relative to Executable Path

The function calculates the resource location relative to where the `lumos` executable is installed.

**Expected structure:**
```
/usr/local/
├── bin/
│   └── lumos              # Executable
└── share/
    └── lumos/             # Resources (LUMOS_ROOT)
        ├── boards/
        ├── hal/
        └── platforms/
```

**Platform-specific implementations:**

**macOS:**
```cpp
_NSGetExecutablePath(exe_path, &size)
// Returns: /usr/local/bin/lumos
// Calculates: /usr/local/share/lumos
```

**Linux:**
```cpp
readlink("/proc/self/exe", exe_path, ...)
// Returns: /usr/local/bin/lumos
// Calculates: /usr/local/share/lumos
```

**Windows:**
```cpp
GetModuleFileNameA(NULL, exe_path, MAX_PATH)
// Returns: C:\Program Files\Lumos\bin\lumos.exe
// Calculates: C:\Program Files\Lumos\share\lumos
```

---

### 3. Standard Installation Locations

If relative path resolution fails, check common installation directories:

```cpp
/usr/local/share/lumos  // Most common (Homebrew, manual install)
/usr/share/lumos        // System-wide installation
/opt/lumos/share/lumos  // Alternative location
```

---

### 4. Development Fallback (Lowest Priority)

For developers running from the build directory:

```cpp
/Users/danielpi/work/LumosTool  // Hardcoded development path
```

This allows development without installation.

---

## Distribution Scenarios

### Scenario 1: macOS Homebrew Installation
```
/usr/local/
├── bin/lumos
└── share/lumos/boards/...
```
**Resolution:** Priority 2 (relative to executable)

### Scenario 2: Linux Package (.deb, .rpm)
```
/usr/
├── bin/lumos
└── share/lumos/boards/...
```
**Resolution:** Priority 2 or 3

### Scenario 3: Portable Installation
```
~/Applications/lumos/
├── bin/lumos
└── share/lumos/boards/...
```
**Resolution:** Priority 2 (relative to executable)

### Scenario 4: Custom Installation
```bash
export LUMOS_ROOT=/custom/location/lumos
./lumos build
```
**Resolution:** Priority 1 (environment variable)

### Scenario 5: Development
```
/Users/danielpi/work/LumosTool/
├── build/src/applications/lumos_simple/lumos
├── boards/
├── hal/
└── platforms/
```
**Resolution:** Priority 4 (development fallback)

---

## Error Handling

If all resolution methods fail, `GetLumosRoot()` returns an empty string.

The builder will then display an error:
```
Error: Could not find Lumos installation directory
Please set LUMOS_ROOT environment variable
```

---

## Testing Path Resolution

### Test in development:
```bash
cd /path/to/project
/Users/danielpi/work/LumosTool/build/src/applications/lumos_simple/lumos build
```
Should use development fallback.

### Test after installation:
```bash
sudo cp lumos /usr/local/bin/
sudo cp -r boards /usr/local/share/lumos/
lumos build
```
Should use relative path resolution.

### Test with override:
```bash
export LUMOS_ROOT=/custom/path
lumos build
```
Should use custom path.

---

## Implementation Details

### Platform-Specific Headers

**macOS:**
```cpp
#include <mach-o/dyld.h>  // _NSGetExecutablePath
#include <limits.h>        // PATH_MAX
```

**Linux:**
```cpp
#include <unistd.h>       // readlink
#include <limits.h>       // PATH_MAX
```

**Windows:**
```cpp
#include <windows.h>      // GetModuleFileNameA, MAX_PATH
```

### Code Location

File: `src/applications/lumos_simple/main.cpp`
Function: `GetLumosRoot()` (lines 56-125)

---

## Future Improvements

1. **XDG Base Directory Support** (Linux)
   - Check `~/.local/share/lumos`
   - Check `$XDG_DATA_HOME/lumos`

2. **Registry Support** (Windows)
   - Store installation path in registry
   - `HKLM\Software\Lumos\InstallPath`

3. **Improved Error Messages**
   - Show which paths were checked
   - Suggest installation commands

4. **Validation**
   - Verify boards/ directory exists
   - Check for minimum required files
   - Warn if resources are incomplete
