# Build Configuration

## LUMOS_OFFICIAL_RELEASE Flag

The `LUMOS_OFFICIAL_RELEASE` CMake option controls whether the build is for development or official release distribution.

### Default Behavior

- **Development builds** (default): `LUMOS_OFFICIAL_RELEASE=0`
  - Allows `LUMOS_ROOT` environment variable override
  - More flexible for testing and development
  - Built with: `cmake -S . -B build`

- **Official releases**: `LUMOS_OFFICIAL_RELEASE=1`
  - Prioritizes installed paths
  - Ignores environment variable (unless specifically needed)
  - Built with: `cmake -S . -B build -DLUMOS_OFFICIAL_RELEASE=ON`

### Usage in CMake

The flag is automatically set in the release build script:

```bash
# Development build
cmake -S . -B build

# Official release build
cmake -S . -B build -DLUMOS_OFFICIAL_RELEASE=ON
```

### Usage in C++ Code

The macro is available in all C++ files as a compile-time constant:

```cpp
#if LUMOS_OFFICIAL_RELEASE
    // Code for official releases
    std::cout << "Running official release build" << std::endl;
#else
    // Code for development builds
    std::cout << "Running development build" << std::endl;
#endif
```

### Example: Conditional Behavior

```cpp
std::string GetConfigPath() {
#if LUMOS_OFFICIAL_RELEASE
    // Official release: use system paths only
    return "/usr/local/share/lumos/config";
#else
    // Development: allow override for testing
    const char* custom_path = std::getenv("LUMOS_CONFIG_PATH");
    if (custom_path) {
        return std::string(custom_path);
    }
    return "./config";  // Local development path
#endif
}
```

### Build Scripts

The `scripts/create_macos_release.py` script automatically sets this flag when building release packages:

```python
cmake_flags = [
    "-DCMAKE_BUILD_TYPE=Release",
    "-DLUMOS_OFFICIAL_RELEASE=ON",  # Mark as official release
    # ... other flags
]
```

### Verification

Check which mode your build is in:

```bash
# Look for the message during CMake configuration
cmake -S . -B build
# Output: "Building DEVELOPMENT version"

cmake -S . -B build -DLUMOS_OFFICIAL_RELEASE=ON
# Output: "Building OFFICIAL RELEASE"
```

You can also check at runtime in your code:

```cpp
void PrintBuildInfo() {
#if LUMOS_OFFICIAL_RELEASE
    std::cout << "Build type: Official Release" << std::endl;
#else
    std::cout << "Build type: Development" << std::endl;
#endif
}
```
