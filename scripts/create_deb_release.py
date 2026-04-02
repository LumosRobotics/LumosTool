#!/usr/bin/env python3
"""
Create a Debian (.deb) package for Lumos.

Reuses the Linux glibc release package (built by create_linux_release.py) and
repackages it as a proper .deb so users on Debian/Ubuntu can install via apt.

This script:
1. Builds the Linux release package if not already present
2. Assembles the .deb directory tree
3. Runs dpkg-deb inside a Docker container (works from macOS or Linux)
4. Outputs lumos_{VERSION}_amd64.deb to release/
"""

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

# Configuration
VERSION = "1.0.0"
PACKAGE = "lumos"
ARCHITECTURE = "amd64"
MAINTAINER = "LumosRobotics <support@lumosrobotics.com>"
HOMEPAGE = "https://github.com/LumosRobotics/LumosTool"
DESCRIPTION_SHORT = "CLI build tool for STM32 embedded systems"
DESCRIPTION_LONG = """\
 Lumos is a CLI toolchain for building, flashing, and monitoring
 STM32-based embedded systems. Supports the lumos_brain (STM32H7),
 lumos_micro_brain (STM32G0), and sx1281_module (STM32H5) boards
 with an Arduino-compatible API.
 .
 Includes the ARM GCC cross-compiler (gcc-arm-none-eabi 10.3-2021.10)."""

PROJECT_ROOT = Path(__file__).parent.parent.absolute()
RELEASE_DIR = PROJECT_ROOT / "release"
LINUX_PACKAGE_DIR = RELEASE_DIR / f"lumos-linux-{VERSION}"
DEB_NAME = f"{PACKAGE}_{VERSION}_{ARCHITECTURE}.deb"
DEB_PATH = RELEASE_DIR / DEB_NAME


def print_step(message):
    print(f"\n{'='*60}")
    print(f"  {message}")
    print(f"{'='*60}\n")


def run_command(cmd, cwd=None, capture=True):
    print(f"Running: {' '.join(str(c) for c in cmd)}")
    result = subprocess.run(cmd, cwd=cwd, capture_output=capture, text=True)
    if result.returncode != 0:
        print(f"Error: {' '.join(str(c) for c in cmd)}")
        if capture:
            print(f"stdout: {result.stdout}")
            print(f"stderr: {result.stderr}")
        sys.exit(1)
    return result


def check_requirements():
    print_step("Checking Requirements")

    result = subprocess.run(["which", "docker"], capture_output=True)
    if result.returncode != 0:
        print("Error: docker not found.")
        sys.exit(1)
    print("✓ Found docker")

    result = subprocess.run(["docker", "info"], capture_output=True, text=True)
    if result.returncode != 0:
        print("Error: Docker daemon is not running.")
        sys.exit(1)
    print("✓ Docker daemon is running")
    print("✓ All requirements met")


def ensure_linux_release():
    """Build the Linux glibc release package if it doesn't already exist."""
    print_step("Checking Linux Release Package")

    if (LINUX_PACKAGE_DIR / "bin" / "lumos").exists():
        print(f"✓ Linux release package found at {LINUX_PACKAGE_DIR}")
        return

    print("Linux release package not found — building it now...")
    run_command(
        [sys.executable, str(PROJECT_ROOT / "scripts" / "create_linux_release.py")],
        capture=False,
    )


def assemble_deb_tree(deb_root):
    """Build the .deb directory tree from the Linux release package."""
    print_step("Assembling .deb Directory Tree")

    # Install paths
    usr_bin = deb_root / "usr" / "bin"
    usr_share = deb_root / "usr" / "share" / "lumos"
    debian_dir = deb_root / "DEBIAN"

    usr_bin.mkdir(parents=True)
    usr_share.mkdir(parents=True)
    debian_dir.mkdir(parents=True)

    # Copy lumos binary
    shutil.copy2(LINUX_PACKAGE_DIR / "bin" / "lumos", usr_bin / "lumos")
    (usr_bin / "lumos").chmod(0o755)
    print("  ✓ bin/lumos")

    # Copy share contents (boards, toolchains)
    src_share = LINUX_PACKAGE_DIR / "share"
    if src_share.exists():
        for item in src_share.iterdir():
            dst = usr_share / item.name
            if item.is_dir():
                shutil.copytree(item, dst, symlinks=True)
            else:
                shutil.copy2(item, dst)
            size_mb = sum(
                f.stat().st_size for f in dst.rglob("*") if f.is_file()
            ) / (1024 * 1024) if dst.is_dir() else dst.stat().st_size / (1024 * 1024)
            print(f"  ✓ share/{item.name} ({size_mb:.0f} MB)")

    # Calculate installed size (in KB, as required by dpkg)
    installed_kb = sum(
        f.stat().st_size for f in deb_root.rglob("*") if f.is_file()
    ) // 1024

    # Write DEBIAN/control
    control = f"""\
Package: {PACKAGE}
Version: {VERSION}
Architecture: {ARCHITECTURE}
Maintainer: {MAINTAINER}
Installed-Size: {installed_kb}
Homepage: {HOMEPAGE}
Section: devel
Priority: optional
Description: {DESCRIPTION_SHORT}
{DESCRIPTION_LONG}
"""
    (debian_dir / "control").write_text(control)
    print("  ✓ DEBIAN/control")

    # Write DEBIAN/postinst (set permissions)
    postinst = """\
#!/bin/sh
chmod +x /usr/bin/lumos
"""
    postinst_path = debian_dir / "postinst"
    postinst_path.write_text(postinst)
    postinst_path.chmod(0o755)
    print("  ✓ DEBIAN/postinst")

    print(f"\n✓ .deb tree assembled ({installed_kb / 1024:.0f} MB installed)")
    return deb_root


def build_deb(deb_root):
    """Run dpkg-deb inside an Ubuntu container to produce the .deb file."""
    print_step(f"Building {DEB_NAME}")

    if DEB_PATH.exists():
        DEB_PATH.unlink()

    # Use an Ubuntu container so dpkg-deb is available (works on macOS too)
    run_command([
        "docker", "run", "--rm",
        "-v", f"{deb_root}:/pkg",
        "-v", f"{RELEASE_DIR}:/out",
        "ubuntu:22.04",
        "dpkg-deb", "--build", "--root-owner-group", "/pkg", f"/out/{DEB_NAME}",
    ])

    size_mb = DEB_PATH.stat().st_size / (1024 * 1024)
    print(f"✓ Created {DEB_NAME} ({size_mb:.1f} MB)")


def calculate_checksum():
    print_step("Calculating Checksum")

    if platform.system() == "Darwin":
        result = run_command(["shasum", "-a", "256", DEB_PATH.name], cwd=RELEASE_DIR)
    else:
        result = run_command(["sha256sum", DEB_PATH.name], cwd=RELEASE_DIR)

    checksum_line = result.stdout.strip()
    checksum_file = RELEASE_DIR / f"{DEB_NAME}.sha256"
    checksum_file.write_text(checksum_line + "\n")

    print(f"SHA256: {checksum_line}")
    print(f"✓ Saved to: {checksum_file.name}")
    return checksum_line


def main():
    print(f"""
    ╔════════════════════════════════════════════════════════╗
    ║        Lumos Debian Package Builder v{VERSION}           ║
    ╚════════════════════════════════════════════════════════╝
    """)

    try:
        check_requirements()
        ensure_linux_release()

        deb_root = RELEASE_DIR / f"{PACKAGE}_{VERSION}_{ARCHITECTURE}"
        if deb_root.exists():
            shutil.rmtree(deb_root)

        assemble_deb_tree(deb_root)
        build_deb(deb_root)
        shutil.rmtree(deb_root)  # clean up staging dir

        checksum = calculate_checksum()

        print_step("Build Complete!")
        print(f"Package:  {DEB_PATH}")
        print(f"SHA256:   {checksum.split()[0]}")
        print(f"\nTo test:")
        print(f"  sudo dpkg -i {DEB_PATH}")
        print(f"  lumos --version")
        print(f"\nTo distribute:")
        print(f"  Upload {DEB_NAME} to GitHub releases")

    except KeyboardInterrupt:
        print("\n\nBuild interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n\nError: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()
