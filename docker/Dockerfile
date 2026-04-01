FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# ── System packages ────────────────────────────────────────────────────────────
RUN apt-get update && apt-get install -y \
        # Core build tools
        build-essential \
        cmake \
        ninja-build \
        git \
        # ARM cross-compiler for firmware builds
        gcc-arm-none-eabi \
        binutils-arm-none-eabi \
        libnewlib-arm-none-eabi \
        # Misc utilities
        python3 \
        wget \
        curl \
    && rm -rf /var/lib/apt/lists/*

# ── Workspace ──────────────────────────────────────────────────────────────────
# Mount the repo here at runtime:
#   docker run --rm -v /path/to/LumosTool:/workspace lumos-build bash
WORKDIR /workspace

# Helper script: builds third-party deps (yaml-cpp, googletest) from submodules.
# Run once after mounting the repo and initialising submodules.
COPY setup_deps.sh /usr/local/bin/setup_deps
RUN chmod +x /usr/local/bin/setup_deps

CMD ["/bin/bash"]
