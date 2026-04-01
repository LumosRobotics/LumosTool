#!/usr/bin/env bash
# Builds third-party dependencies from submodule sources.
# Run this once inside the container after mounting the repo:
#   setup_deps
# The repo must be mounted at /workspace and submodules must be initialised.

set -euo pipefail

REPO=/workspace
JOBS=$(nproc)

# ── yaml-cpp ───────────────────────────────────────────────────────────────────
YAML_SRC="$REPO/third_party/yaml-cpp"
YAML_BUILD="$YAML_SRC/build"

if [ ! -f "$YAML_SRC/CMakeLists.txt" ]; then
    echo "ERROR: yaml-cpp submodule not found at $YAML_SRC"
    echo "Run: git submodule update --init third_party/yaml-cpp"
    exit 1
fi

echo "==> Building yaml-cpp ..."
cmake -S "$YAML_SRC" -B "$YAML_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DYAML_CPP_BUILD_TESTS=OFF \
    -DYAML_CPP_BUILD_TOOLS=OFF \
    -DYAML_BUILD_SHARED_LIBS=OFF
cmake --build "$YAML_BUILD" -j"$JOBS"
echo "    yaml-cpp built -> $YAML_BUILD/libyaml-cpp.a"

# ── googletest ─────────────────────────────────────────────────────────────────
GTEST_SRC="$REPO/third_party/googletest"
GTEST_BUILD="$GTEST_SRC/build"
GTEST_LIB="$GTEST_SRC/lib"

if [ ! -f "$GTEST_SRC/CMakeLists.txt" ]; then
    echo "ERROR: googletest submodule not found at $GTEST_SRC"
    echo "Run: git submodule update --init third_party/googletest"
    exit 1
fi

echo "==> Building googletest ..."
cmake -S "$GTEST_SRC" -B "$GTEST_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_GMOCK=OFF
cmake --build "$GTEST_BUILD" -j"$JOBS"

mkdir -p "$GTEST_LIB"
cp "$GTEST_BUILD/lib/libgtest.a"      "$GTEST_LIB/"
cp "$GTEST_BUILD/lib/libgtest_main.a" "$GTEST_LIB/"
echo "    googletest built -> $GTEST_LIB/"

echo ""
echo "Done. You can now configure and build the project:"
echo "  cmake -S /workspace -B /workspace/build/linux"
echo "  cmake --build /workspace/build/linux -j\$(nproc)"
