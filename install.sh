#!/usr/bin/env bash
# Lumos installer
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/LumosRobotics/LumosTool/main/install.sh | bash
#
# Environment variables:
#   LUMOS_VERSION      Install a specific version (default: latest)
#   INSTALL_PREFIX     Install destination (default: /usr/local)

set -euo pipefail

REPO="LumosRobotics/LumosTool"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

# ── Helpers ───────────────────────────────────────────────────────────────────

print_step() { printf "\n\033[1;34m==>\033[0m \033[1m%s\033[0m\n" "$1"; }
print_ok()   { printf "  \033[1;32m✓\033[0m %s\n" "$1"; }
print_warn() { printf "  \033[1;33m!\033[0m %s\n" "$1"; }
print_err()  { printf "\n\033[1;31mError:\033[0m %s\n" "$1" >&2; }

die() { print_err "$1"; exit 1; }

need() {
    command -v "$1" &>/dev/null || die "'$1' is required but not installed."
}

# ── Platform detection ────────────────────────────────────────────────────────

detect_os() {
    case "$(uname -s)" in
        Darwin) echo "macos" ;;
        Linux)  echo "linux" ;;
        *)      die "Unsupported operating system: $(uname -s)" ;;
    esac
}

detect_arch() {
    case "$(uname -m)" in
        x86_64 | amd64) echo "x86_64" ;;
        *) die "Unsupported architecture: $(uname -m). Only x86_64 is currently supported." ;;
    esac
}

# Returns "musl" or "glibc"
detect_libc() {
    # Check ldd output for musl signature
    if ldd --version 2>&1 | grep -qi musl; then
        echo "musl"; return
    fi
    # Check for musl dynamic linker on disk
    if [ -e /lib/ld-musl-x86_64.so.1 ]; then
        echo "musl"; return
    fi
    echo "glibc"
}

# ── Version resolution ────────────────────────────────────────────────────────

latest_version() {
    curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest" \
        | grep '"tag_name"' \
        | sed 's/.*"tag_name": *"v\([^"]*\)".*/\1/'
}

# ── Download + verify ─────────────────────────────────────────────────────────

download_and_verify() {
    local url="$1"
    local sha256_url="${url}.sha256"
    local dest="$2"

    print_step "Downloading $(basename "$url")"
    curl -fsSL --progress-bar "$url" -o "$dest"

    # Verify checksum if available
    if curl -fsSL "$sha256_url" -o "${dest}.sha256" 2>/dev/null; then
        print_step "Verifying checksum"
        local expected actual
        expected=$(awk '{print $1}' "${dest}.sha256")

        if command -v sha256sum &>/dev/null; then
            actual=$(sha256sum "$dest" | awk '{print $1}')
        elif command -v shasum &>/dev/null; then
            actual=$(shasum -a 256 "$dest" | awk '{print $1}')
        else
            print_warn "No sha256 tool found — skipping checksum verification"
            return
        fi

        if [ "$expected" != "$actual" ]; then
            die "Checksum mismatch!\n  expected: $expected\n  got:      $actual"
        fi
        print_ok "Checksum verified"
    else
        print_warn "No checksum file found — skipping verification"
    fi
}

# ── Install ───────────────────────────────────────────────────────────────────

install_files() {
    local package_dir="$1"

    print_step "Installing to $INSTALL_PREFIX"

    local need_sudo=""
    if [ ! -w "$INSTALL_PREFIX" ]; then
        if command -v sudo &>/dev/null; then
            print_warn "$INSTALL_PREFIX is not writable — using sudo"
            need_sudo="sudo"
        else
            die "$INSTALL_PREFIX is not writable and sudo is not available.\nRe-run with: INSTALL_PREFIX=~/.local $0"
        fi
    fi

    $need_sudo mkdir -p "$INSTALL_PREFIX/bin" "$INSTALL_PREFIX/share/lumos"
    $need_sudo cp -f "$package_dir/bin/lumos" "$INSTALL_PREFIX/bin/lumos"
    $need_sudo chmod +x "$INSTALL_PREFIX/bin/lumos"
    $need_sudo cp -rf "$package_dir/share/"* "$INSTALL_PREFIX/share/lumos/"

    print_ok "Installed $INSTALL_PREFIX/bin/lumos"
}

# ── PATH check ────────────────────────────────────────────────────────────────

check_path() {
    if ! echo "$PATH" | tr ':' '\n' | grep -qx "$INSTALL_PREFIX/bin"; then
        print_warn "$INSTALL_PREFIX/bin is not in your PATH"
        printf "  Add this to your shell config (~/.bashrc, ~/.zshrc, etc.):\n"
        printf "    export PATH=\"%s/bin:\$PATH\"\n" "$INSTALL_PREFIX"
    fi
}

# ── Main ──────────────────────────────────────────────────────────────────────

main() {
    printf "\n\033[1mLumos Installer\033[0m\n"
    printf "%s\n" "─────────────────────────────────"

    need curl
    need tar

    local os arch
    os=$(detect_os)
    arch=$(detect_arch)
    print_ok "Platform: $os ($arch)"

    # Resolve version
    local version="${LUMOS_VERSION:-}"
    if [ -z "$version" ]; then
        print_step "Fetching latest version"
        version=$(latest_version) || die "Could not determine latest version. Set LUMOS_VERSION manually."
    fi
    print_ok "Version: $version"

    # Pick the right archive
    local archive libc=""
    case "$os" in
        macos)
            archive="lumos-macos-${version}.tar.gz"
            ;;
        linux)
            libc=$(detect_libc)
            print_ok "libc: $libc"
            if [ "$libc" = "musl" ]; then
                archive="lumos-linux-musl-${version}.tar.gz"
            else
                archive="lumos-linux-${version}.tar.gz"
            fi
            ;;
    esac

    local base_url="https://github.com/${REPO}/releases/download/v${version}"
    local url="${base_url}/${archive}"

    # Download into a temp dir
    local tmpdir
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT

    local archive_path="$tmpdir/$archive"
    download_and_verify "$url" "$archive_path"

    # Extract
    print_step "Extracting"
    tar -xzf "$archive_path" -C "$tmpdir"
    local package_dir
    package_dir=$(find "$tmpdir" -mindepth 1 -maxdepth 1 -type d | head -1)

    # Install
    install_files "$package_dir"

    # Verify the installed binary runs
    print_step "Verifying installation"
    if "$INSTALL_PREFIX/bin/lumos" --version &>/dev/null; then
        local installed_version
        installed_version=$("$INSTALL_PREFIX/bin/lumos" --version 2>&1 | head -1)
        print_ok "lumos is working: $installed_version"
    else
        print_warn "Could not verify — run 'lumos --version' to check"
    fi

    check_path

    printf "\n\033[1;32mDone!\033[0m Lumos v%s installed.\n\n" "$version"
    printf "Get started:\n"
    printf "  mkdir my_project && cd my_project\n"
    printf "  lumos init\n\n"
}

main "$@"
