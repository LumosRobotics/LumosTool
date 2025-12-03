# Lumos System Tests

End-to-end system tests for the Lumos CLI tool using pytest.

## Overview

These tests execute the actual `lumos_dev` binary and verify:
- Command-line interface behavior
- File generation and project initialization
- Build process execution
- Error handling and edge cases
- User interaction flows

## Prerequisites

1. **Build the project first:**
   ```bash
   cd /Users/danielpi/work/LumosTool
   mkdir -p build && cd build
   cmake ..
   cmake --build .
   ```

   This creates the `lumos_dev` binary at `build/src/applications/lumos_simple/lumos_dev`

2. **Install test dependencies:**
   ```bash
   pip3 install -r tests/system/requirements.txt
   ```

## Running Tests

### Run All System Tests
```bash
cd /Users/danielpi/work/LumosTool
pytest tests/system/ -v
```

### Run Specific Test File
```bash
pytest tests/system/test_init.py -v
```

### Run Specific Test
```bash
pytest tests/system/test_init.py::TestLumosInit::test_init_creates_cpp_project_with_defaults -v
```

### Run with Output Capture Disabled (see print statements)
```bash
pytest tests/system/ -v -s
```

### Run with Coverage
```bash
pytest tests/system/ --cov=src/applications/lumos_simple --cov-report=html
```

## Test Structure

```
tests/system/
├── conftest.py              # Pytest fixtures and configuration
├── test_init.py             # Tests for 'lumos init' command
├── test_build.py            # Tests for 'lumos build' command (TODO)
├── test_version.py          # Tests for version/help commands (TODO)
└── requirements.txt         # Python dependencies
```

## Writing New Tests

### Test Fixtures Available

- **`lumos_root`**: Path to the Lumos repository root
- **`lumos_binary`**: Path to the `lumos_dev` binary (skips if not found)
- **`temp_project_dir`**: Temporary directory for testing, auto-cleanup
- **`run_lumos(args, input_text)`**: Helper function to execute lumos commands

### Example Test

```python
def test_my_feature(temp_project_dir, run_lumos):
    """Test description"""
    # Run lumos init with C++ defaults
    result = run_lumos(['init'], input_text="\n\n")

    # Verify success
    assert result.returncode == 0
    assert "success" in result.stdout

    # Check files created
    assert (temp_project_dir / "main.cpp").exists()
```

## Test Coverage

### Current Coverage

- ✅ `lumos init` - Complete
  - Default C++ project creation
  - C project creation
  - Board selection
  - Re-initialization scenarios
  - File content validation
  - Error handling

### Planned Coverage

- ⏳ `lumos build` - TODO
- ⏳ `lumos --version` / `--help` - TODO
- ⏳ `lumos ports` - TODO
- ⏳ Invalid commands - TODO
- ⏳ Integration workflows (init → build) - TODO

## Continuous Integration

These tests can be integrated into CI/CD pipelines:

```yaml
# .github/workflows/test.yml
- name: Build Lumos
  run: |
    mkdir build && cd build
    cmake ..
    cmake --build .

- name: Run System Tests
  run: |
    pip3 install -r tests/system/requirements.txt
    pytest tests/system/ -v
```

## Debugging Failed Tests

1. **Check test output:**
   ```bash
   pytest tests/system/ -v -s
   ```

2. **Run single test with full output:**
   ```bash
   pytest tests/system/test_init.py::test_name -vv -s
   ```

3. **Inspect temporary directory:**
   ```bash
   pytest tests/system/ --basetemp=/tmp/lumos_tests
   # Files will be in /tmp/lumos_tests/
   ```

4. **Check binary exists:**
   ```bash
   ls -la build/src/applications/lumos_simple/lumos_dev
   ```

## Notes

- Tests use temporary directories that are automatically cleaned up
- Each test runs in isolation with a fresh project directory
- The `lumos_dev` binary must be built before running tests
- Tests simulate user input via stdin for interactive commands
- All file paths are cross-platform compatible using `pathlib`
