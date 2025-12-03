# Lumos Test Suite

Comprehensive testing infrastructure for the Lumos build tool.

## Test Types

### System Tests (`tests/system/`)
End-to-end tests that execute the actual `lumos_dev` binary and verify complete workflows.

**What they test:**
- CLI commands (`init`, `build`, `flash`, etc.)
- File generation and project initialization
- User interaction flows
- Error handling
- Integration between components

**Quick start:**
```bash
# Install dependencies
pip3 install -r tests/system/requirements.txt

# Build the project first
mkdir -p build && cd build
cmake ..
cmake --build .

# Run tests
cd /Users/danielpi/work/LumosTool
pytest tests/system/ -v

# Or use the helper script
./tests/system/run_tests.sh
```

See [tests/system/README.md](system/README.md) for detailed documentation.

### Unit Tests (TODO)
Component-level tests for individual classes and functions.

**Planned coverage:**
- `ProjectConfig` parsing
- `Builder` compilation logic
- `HALModuleDetector` detection
- Path resolution
- Configuration validation

## Running Tests

### All Tests
```bash
pytest tests/ -v
```

### System Tests Only
```bash
pytest tests/system/ -v
```

### Specific Test File
```bash
pytest tests/system/test_init.py -v
```

### With Coverage
```bash
pytest tests/ --cov=src --cov-report=html --cov-report=term
open htmlcov/index.html  # View coverage report
```

## Test Infrastructure

### Fixtures (`tests/system/conftest.py`)
- `lumos_root` - Repository root path
- `lumos_binary` - Path to `lumos_dev` binary
- `temp_project_dir` - Clean temporary directory for each test
- `run_lumos(args, input_text)` - Execute lumos commands with input

### Helper Scripts
- `tests/system/run_tests.sh` - Convenient test runner with setup validation

## Continuous Integration

Add to `.github/workflows/test.yml`:

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: macos-latest

    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: |
          pip3 install -r tests/system/requirements.txt

      - name: Build Lumos
        run: |
          mkdir build && cd build
          cmake ..
          cmake --build .

      - name: Run system tests
        run: |
          pytest tests/system/ -v

      - name: Upload coverage
        if: always()
        uses: codecov/codecov-action@v3
```

## Writing Tests

### System Test Example

```python
def test_init_creates_project(temp_project_dir, run_lumos):
    """Test that init creates a new project"""
    # Run command with simulated user input
    result = run_lumos(['init'], input_text="\n\n")

    # Verify success
    assert result.returncode == 0

    # Check files created
    assert (temp_project_dir / "main.cpp").exists()
    assert (temp_project_dir / "project.yaml").exists()

    # Verify content
    content = (temp_project_dir / "main.cpp").read_text()
    assert "void setup()" in content
```

## Test Coverage Status

| Component | System Tests | Unit Tests | Coverage |
|-----------|--------------|------------|----------|
| `lumos init` | ✅ Complete | ⏳ Planned | 90%+ |
| `lumos --version/--help` | ✅ Complete | ⏳ Planned | 100% |
| `lumos build` | ⏳ Planned | ⏳ Planned | 0% |
| `lumos flash` | ⏳ Planned | ⏳ Planned | 0% |
| `lumos monitor` | ⏳ Planned | ⏳ Planned | 0% |
| `lumos ports` | ⏳ Planned | ⏳ Planned | 0% |
| `ProjectConfig` | N/A | ⏳ Planned | 0% |
| `Builder` | N/A | ⏳ Planned | 0% |

## Debugging Failed Tests

### Enable verbose output
```bash
pytest tests/system/ -vv -s
```

### Run single test
```bash
pytest tests/system/test_init.py::TestLumosInit::test_specific_test -vv -s
```

### Inspect temporary files
```bash
pytest tests/system/ --basetemp=/tmp/lumos_tests
ls -la /tmp/lumos_tests/
```

### Check binary
```bash
ls -la build/src/applications/lumos_simple/lumos_dev
./build/src/applications/lumos_simple/lumos_dev --version
```

## Dependencies

Install all test dependencies:
```bash
pip3 install -r tests/system/requirements.txt
```

Current dependencies:
- `pytest>=7.0.0` - Test framework
- `pytest-timeout>=2.1.0` - Timeout handling
- `pyyaml>=6.0` - YAML parsing for validation

## Contributing

When adding new features:

1. **Write system tests first** (TDD approach)
2. Test both success and error cases
3. Use descriptive test names
4. Add docstrings explaining what's being tested
5. Run full test suite before committing

```bash
# Before committing
./tests/system/run_tests.sh
```
