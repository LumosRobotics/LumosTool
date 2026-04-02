# Installing Lumos

## macOS

The easiest way to install on macOS is via Homebrew:

```bash
brew install LumosRobotics/tools/lumos
```

## Linux — Debian/Ubuntu (.deb package)

Download `lumos_{VERSION}_amd64.deb` from the [releases page](https://github.com/LumosRobotics/LumosTool/releases) and install with:

```bash
sudo dpkg -i lumos_1.0.0_amd64.deb
```

This installs `lumos` to `/usr/bin/lumos` and support files to `/usr/share/lumos/`, including the bundled ARM toolchain.

## Linux — Fedora/RHEL/CentOS/openSUSE (.rpm package)

Download `lumos-{VERSION}-1.x86_64.rpm` from the [releases page](https://github.com/LumosRobotics/LumosTool/releases) and install with:

```bash
sudo rpm -i lumos-1.0.0-1.x86_64.rpm          # Fedora / RHEL / CentOS
sudo zypper install lumos-1.0.0-1.x86_64.rpm  # openSUSE
```

## Linux and macOS — install script

A script is provided that automatically detects your platform and installs the right binary.

### Quick install

```bash
curl -fsSL https://raw.githubusercontent.com/LumosRobotics/LumosTool/main/install.sh | bash
```

### Verified install (recommended for the security-conscious)

Pin to a specific commit and verify the script's checksum before running it:

```bash
curl -fsSL https://raw.githubusercontent.com/LumosRobotics/LumosTool/<COMMIT>/install.sh -o install.sh
echo "<SHA256>  install.sh" | sha256sum -c
bash install.sh
```

The commit SHA and script checksum for each release are published in the [release notes](https://github.com/LumosRobotics/LumosTool/releases).

### Custom install prefix

By default Lumos installs to `/usr/local`. To install elsewhere (e.g. without sudo):

```bash
INSTALL_PREFIX=~/.local curl -fsSL https://raw.githubusercontent.com/LumosRobotics/LumosTool/main/install.sh | bash
```

Make sure the `bin` directory inside your prefix is in `$PATH`:

```bash
export PATH="$HOME/.local/bin:$PATH"
```

### Install a specific version

```bash
LUMOS_VERSION=1.0.0 curl -fsSL https://raw.githubusercontent.com/LumosRobotics/LumosTool/main/install.sh | bash
```

## Linux — platform notes

The install script automatically picks the right binary for your system:

| Your system | Binary used |
|---|---|
| Ubuntu, Debian, Fedora, RHEL, Arch, and most mainstream distros | `lumos-linux-{VERSION}.tar.gz` (glibc 2.17+) |
| Alpine Linux, Void Linux, or any musl-based distro | `lumos-linux-musl-{VERSION}.tar.gz` (fully static) |

If you are unsure which applies to you, the script detects it automatically.

## Manual installation

All release archives are available on the [releases page](https://github.com/LumosRobotics/LumosTool/releases). Each archive contains:

```
lumos-{platform}-{version}/
├── bin/
│   └── lumos          # (or lumos.exe on Windows)
├── share/
│   ├── boards/        # STM32 board definitions
│   └── toolchains/    # Platform toolchain files
├── install.sh         # (or install.bat on Windows)
└── README.txt
```

Run `install.sh` (or `install.bat` on Windows) from the extracted directory, or copy the `bin/lumos` binary anywhere on your `$PATH` and set `LUMOS_ROOT` to the `share/` directory.

## Windows

Download `lumos-windows-{VERSION}.zip` from the [releases page](https://github.com/LumosRobotics/LumosTool/releases), extract it, and run `install.bat` as Administrator:

```
Right-click install.bat → "Run as administrator"
```

This installs Lumos to `%ProgramFiles%\Lumos` and adds it to your PATH. To install to a custom location:

```bat
install.bat "C:\MyTools\Lumos"
```

## ARM toolchain

The ARM GCC cross-compiler (`gcc-arm-none-eabi 10.3-2021.10`) is **bundled** in all release packages — macOS, Linux, and Windows. No separate installation is required.

## Verifying your installation

```bash
lumos --version
```

## Getting started

```bash
mkdir my_project && cd my_project
lumos init
lumos build
lumos flash
```
