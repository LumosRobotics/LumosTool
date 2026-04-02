#!/usr/bin/env python3
"""
Create an RPM (.rpm) package for Lumos.

Reuses the Linux glibc release package (built by create_linux_release.py) and
repackages it as a proper .rpm for Fedora, RHEL, CentOS, openSUSE, and Arch.

This script:
1. Builds the Linux release package if not already present
2. Assembles the rpmbuild directory tree and writes a .spec file
3. Runs rpmbuild inside a Fedora Docker container (works from macOS or Linux)
4. Outputs lumos-{VERSION}-1.x86_64.rpm to release/
"""

import platform
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path

# Configuration
VERSION = "1.0.0"
RELEASE = "1"
PACKAGE = "lumos"
ARCHITECTURE = "x86_64"
SUMMARY = "CLI build tool for STM32 embedded systems"
LICENSE = "MIT"
URL = "https://github.com/LumosRobotics/LumosTool"
DESCRIPTION = """\
Lumos is a CLI toolchain for building, flashing, and monitoring
STM32-based embedded systems. Supports the lumos_brain (STM32H7),
lumos_micro_brain (STM32G0), and sx1281_module (STM32H5) boards
with an Arduino-compatible API.

Includes the ARM GCC cross-compiler (gcc-arm-none-eabi 10.3-2021.10)."""

PROJECT_ROOT = Path(__file__).parent.parent.absolute()
RELEASE_DIR = PROJECT_ROOT / "release"
LINUX_PACKAGE_DIR = RELEASE_DIR / f"lumos-linux-{VERSION}"
RPM_NAME = f"{PACKAGE}-{VERSION}-{RELEASE}.{ARCHITECTURE}.rpm"
RPM_PATH = RELEASE_DIR / RPM_NAME


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


def create_source_tarball(build_dir):
    """Create a source tarball of the lumos binary + share files for rpmbuild."""
    print_step("Creating Source Tarball")

    source_name = f"{PACKAGE}-{VERSION}"
    source_dir = build_dir / "SOURCES" / source_name
    source_dir.mkdir(parents=True)

    # Copy binary
    shutil.copy2(LINUX_PACKAGE_DIR / "bin" / "lumos", source_dir / "lumos")
    (source_dir / "lumos").chmod(0o755)
    print("  ✓ bin/lumos")

    # Copy share contents
    src_share = LINUX_PACKAGE_DIR / "share"
    if src_share.exists():
        dst_share = source_dir / "share"
        shutil.copytree(src_share, dst_share, symlinks=True)
        for item in dst_share.iterdir():
            size_mb = sum(
                f.stat().st_size for f in item.rglob("*") if f.is_file()
            ) / (1024 * 1024) if item.is_dir() else item.stat().st_size / (1024 * 1024)
            print(f"  ✓ share/{item.name} ({size_mb:.0f} MB)")

    # Pack into tarball
    tarball_path = build_dir / "SOURCES" / f"{source_name}.tar.gz"
    with tarfile.open(tarball_path, "w:gz") as tf:
        tf.add(source_dir, arcname=source_name)
    shutil.rmtree(source_dir)

    print(f"✓ Source tarball: {tarball_path.name}")
    return tarball_path


def write_spec_file(build_dir):
    """Write the RPM .spec file."""
    print_step("Writing .spec File")

    spec_dir = build_dir / "SPECS"
    spec_dir.mkdir(parents=True)
    spec_path = spec_dir / f"{PACKAGE}.spec"

    spec = f"""\
Name:       {PACKAGE}
Version:    {VERSION}
Release:    {RELEASE}%{{?dist}}
Summary:    {SUMMARY}
License:    {LICENSE}
URL:        {URL}
Source0:    %{{name}}-%{{version}}.tar.gz
BuildArch:  {ARCHITECTURE}
ExclusiveArch: {ARCHITECTURE}

%description
{DESCRIPTION}

%prep
%setup -q

%install
rm -rf %{{buildroot}}
install -d %{{buildroot}}/usr/bin
install -d %{{buildroot}}/usr/share/lumos

install -m 755 lumos %{{buildroot}}/usr/bin/lumos
cp -r share/* %{{buildroot}}/usr/share/lumos/

%files
/usr/bin/lumos
/usr/share/lumos/

%changelog
* {_rpm_date()} LumosRobotics <support@lumosrobotics.com> - {VERSION}-{RELEASE}
- Release {VERSION}
"""

    spec_path.write_text(spec)
    print(f"✓ Written: {spec_path.name}")
    return spec_path


def _rpm_date():
    """Return current date in RPM changelog format (e.g. Thu Apr 02 2026)."""
    import datetime
    now = datetime.date.today()
    return now.strftime("%a %b %d %Y")


def build_rpm(build_dir):
    """Run rpmbuild inside a Fedora container to produce the .rpm."""
    print_step(f"Building {RPM_NAME}")

    if RPM_PATH.exists():
        RPM_PATH.unlink()

    # Run rpmbuild in Fedora (has rpmbuild available via rpm-build package)
    run_command([
        "docker", "run", "--rm",
        "-v", f"{build_dir}:/root/rpmbuild",
        "-v", f"{RELEASE_DIR}:/out",
        "fedora:latest",
        "bash", "-c",
        f"dnf install -y rpm-build --quiet "
        f"&& rpmbuild --define '_topdir /root/rpmbuild' -bb /root/rpmbuild/SPECS/{PACKAGE}.spec "
        f"&& cp /root/rpmbuild/RPMS/{ARCHITECTURE}/*.rpm /out/",
    ], capture=False)

    if not RPM_PATH.exists():
        print(f"Error: expected {RPM_PATH} was not created.")
        sys.exit(1)

    size_mb = RPM_PATH.stat().st_size / (1024 * 1024)
    print(f"\n✓ Created {RPM_NAME} ({size_mb:.1f} MB)")


def calculate_checksum():
    print_step("Calculating Checksum")

    if platform.system() == "Darwin":
        result = run_command(["shasum", "-a", "256", RPM_PATH.name], cwd=RELEASE_DIR)
    else:
        result = run_command(["sha256sum", RPM_PATH.name], cwd=RELEASE_DIR)

    checksum_line = result.stdout.strip()
    checksum_file = RELEASE_DIR / f"{RPM_NAME}.sha256"
    checksum_file.write_text(checksum_line + "\n")

    print(f"SHA256: {checksum_line}")
    print(f"✓ Saved to: {checksum_file.name}")
    return checksum_line


def main():
    print(f"""
    ╔════════════════════════════════════════════════════════╗
    ║          Lumos RPM Package Builder v{VERSION}            ║
    ╚════════════════════════════════════════════════════════╝
    """)

    try:
        check_requirements()
        ensure_linux_release()

        build_dir = RELEASE_DIR / f"{PACKAGE}-{VERSION}-rpmbuild"
        if build_dir.exists():
            shutil.rmtree(build_dir)
        build_dir.mkdir()

        # Create required rpmbuild subdirectories
        for d in ["BUILD", "BUILDROOT", "RPMS", "SRPMS"]:
            (build_dir / d).mkdir()

        create_source_tarball(build_dir)
        write_spec_file(build_dir)
        build_rpm(build_dir)
        shutil.rmtree(build_dir)  # clean up staging dir

        checksum = calculate_checksum()

        print_step("Build Complete!")
        print(f"Package:  {RPM_PATH}")
        print(f"SHA256:   {checksum.split()[0]}")
        print(f"\nTo test:")
        print(f"  sudo rpm -i {RPM_PATH}       # RHEL/Fedora/CentOS")
        print(f"  sudo zypper install {RPM_PATH}  # openSUSE")
        print(f"  lumos --version")
        print(f"\nTo distribute:")
        print(f"  Upload {RPM_NAME} to GitHub releases")

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
