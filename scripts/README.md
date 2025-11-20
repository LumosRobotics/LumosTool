# Release Scripts

## macOS Release Builder

### Usage

```bash
cd /Users/danielpi/work/LumosTool
python3 scripts/create_macos_release.py
```

### What It Does

1. **Checks Requirements**
   - Verifies you're on macOS
   - Checks for cmake, lipo, and file tools

2. **Builds for Both Architectures**
   - x86_64 (Intel Macs)
   - arm64 (Apple Silicon)

3. **Creates Universal Binaries**
   - Combines both architectures using `lipo`
   - Creates binaries that work on all Macs

4. **Bundles Resources**
   - Copies boards/, hal/, and platforms/
   - Creates proper directory structure

5. **Generates Distribution Files**
   - Creates install.sh script
   - Creates README.txt with instructions
   - Packages everything as .tar.gz

6. **Calculates Checksums**
   - Generates SHA256 hash for verification

### Output

The script creates:

```
release/
└── lumos-macos-1.0.0/
    ├── bin/
    │   ├── lumos (universal binary)
    │   └── simple_serial (universal binary)
    ├── share/
    │   └── lumos/
    │       ├── boards/
    │       ├── hal/
    │       └── platforms/
    ├── install.sh
    └── README.txt

release/lumos-macos-1.0.0.tar.gz
release/lumos-macos-1.0.0.tar.gz.sha256
```

### Testing the Release

```bash
cd release/lumos-macos-1.0.0
./install.sh
lumos --version
```

### Distribution

1. Upload `lumos-macos-1.0.0.tar.gz` to GitHub Releases
2. Include the SHA256 checksum in the release notes
3. Users download, extract, and run `./install.sh`

### Build Time

- First build: ~5-10 minutes (builds everything twice)
- Subsequent builds: ~2-3 minutes (incremental)

### Requirements

- macOS 10.15 or later
- Xcode Command Line Tools
- CMake: `brew install cmake`
- Python 3.6+

### Troubleshooting

**Error: "cmake not found"**
```bash
brew install cmake
```

**Error: "lipo not found"**
```bash
xcode-select --install
```

**Build fails for arm64 on Intel Mac**
- This is expected if you don't have the arm64 toolchain
- The script will fail gracefully
- Consider building on Apple Silicon or using Rosetta

**Build fails for x86_64 on Apple Silicon**
- Install Rosetta: `softwareupdate --install-rosetta`
