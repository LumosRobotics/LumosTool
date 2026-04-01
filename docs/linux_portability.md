# Linux Portability Strategy

## The Problem

Linux binaries are not universally portable. Unlike macOS (where the OS provides stable system frameworks) or Windows (where we statically link the MinGW runtime), Linux binaries typically link dynamically against `glibc` — the C standard library. glibc is **backwards-compatible but not forwards-compatible**:

- A binary built against glibc 2.35 (Ubuntu 22.04) **will not run** on a system with glibc 2.17 (CentOS 7).
- A binary built against glibc 2.17 **will run** on any system with glibc 2.17 or newer.

Additionally, Alpine Linux and some minimal/embedded distributions use **musl libc** instead of glibc entirely. A glibc binary will not run on musl.

## Our Strategy: Two Builds

We ship two Linux binaries to cover the full spectrum of Linux environments.

### 1. `lumos-linux-{VERSION}.tar.gz` — glibc build (mainstream distros)

| Property | Value |
|---|---|
| Builder | `docker/Dockerfile.release` |
| Base image | `quay.io/pypa/manylinux2014_x86_64` (CentOS 7) |
| glibc version | 2.17 |
| Linking | Dynamic (glibc only) |
| Script | `scripts/create_linux_release.py` |

Targets the oldest glibc still in active use (2.17, from 2012). This covers:

- Ubuntu 18.04+ (glibc 2.27+)
- Debian 10+ / Bullseye+ (glibc 2.28+)
- RHEL / CentOS 7+ (glibc 2.17+)
- Fedora 29+
- Arch Linux (rolling, always newer)
- Most other mainstream distros

**Does not cover**: Alpine Linux or any other musl-based distro.

### 2. `lumos-linux-musl-{VERSION}.tar.gz` — static musl build (Alpine + edge cases)

| Property | Value |
|---|---|
| Builder | `docker/Dockerfile.linux-musl` |
| Base image | `alpine:3.19` |
| Linking | Fully static (no shared library dependencies) |
| Script | `scripts/create_linux_musl_release.py` |

A completely self-contained binary. Runs on:

- Alpine Linux
- Void Linux
- Any Linux distro with kernel >= 3.2 regardless of libc
- Minimal/embedded environments
- Containers with no system libraries

**Trade-off**: musl has subtle differences from glibc in locale handling and some edge-case threading behaviour. For a CLI build tool like `lumos` this is not a practical concern.

## Why manylinux2014?

The `manylinux2014` image was created by the Python packaging community to solve exactly this problem. It provides a stable, audited build environment targeting glibc 2.17 with a modern compiler (GCC 11 via devtoolset-11). Any binary built here is guaranteed to load on any manylinux2014-compatible system, which covers the vast majority of Linux servers and desktops in production.

## Why not just build on Ubuntu 20.04?

Ubuntu 20.04 ships glibc 2.31. A binary built there won't run on:
- CentOS 7 / RHEL 7 (glibc 2.17)
- CentOS 8 / RHEL 8 (glibc 2.28)
- Debian 10 Buster (glibc 2.28)
- Ubuntu 18.04 (glibc 2.27)

These are still widely deployed, especially in enterprise and server environments.

## Which build should a user download?

| User's distro | Recommended build |
|---|---|
| Ubuntu / Debian / Fedora / Arch / RHEL 7+ | `lumos-linux-{VERSION}.tar.gz` (glibc) |
| Alpine Linux | `lumos-linux-musl-{VERSION}.tar.gz` (musl) |
| Void Linux | `lumos-linux-musl-{VERSION}.tar.gz` (musl) |
| Unknown / minimal container | `lumos-linux-musl-{VERSION}.tar.gz` (musl) |
| Unsure | Try glibc first; fall back to musl |
