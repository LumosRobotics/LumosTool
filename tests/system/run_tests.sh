#!/bin/bash
#
# Lumos System Test Runner
#
# Convenient script to run system tests with proper setup
#

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/../.."
BINARY_PATH="$REPO_ROOT/build/src/applications/lumos_simple/lumos_dev"

echo "Lumos System Test Runner"
echo "========================"
echo ""

# Check if binary exists
if [ ! -f "$BINARY_PATH" ]; then
    echo -e "${RED}Error: lumos_dev binary not found${NC}"
    echo "Expected location: $BINARY_PATH"
    echo ""
    echo "Please build the project first:"
    echo "  cd $REPO_ROOT"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  cmake --build ."
    echo ""
    exit 1
fi

# Check if binary is executable
if [ ! -x "$BINARY_PATH" ]; then
    echo -e "${YELLOW}Warning: Binary is not executable${NC}"
    echo "Making it executable..."
    chmod +x "$BINARY_PATH"
fi

echo -e "${GREEN}✓ Found lumos_dev binary${NC}"
echo "  Location: $BINARY_PATH"
echo ""

# Check if pytest is installed
if ! command -v pytest &> /dev/null; then
    echo -e "${YELLOW}Warning: pytest not found${NC}"
    echo "Installing test dependencies..."
    pip3 install -r "$SCRIPT_DIR/requirements.txt"
    echo ""
fi

# Parse arguments
PYTEST_ARGS="-v"

if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
    echo "Usage: $0 [options] [test_file]"
    echo ""
    echo "Options:"
    echo "  -v, --verbose     Verbose output (default)"
    echo "  -s, --stdout      Show stdout/stderr from tests"
    echo "  -k PATTERN        Run tests matching PATTERN"
    echo "  --coverage        Generate coverage report"
    echo "  --help            Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                          # Run all tests"
    echo "  $0 test_init.py             # Run specific test file"
    echo "  $0 -k test_cpp              # Run tests matching 'test_cpp'"
    echo "  $0 -s                       # Show all output"
    echo "  $0 --coverage               # Run with coverage"
    echo ""
    exit 0
fi

# Build pytest arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--stdout)
            PYTEST_ARGS="$PYTEST_ARGS -s"
            shift
            ;;
        -k)
            PYTEST_ARGS="$PYTEST_ARGS -k $2"
            shift 2
            ;;
        --coverage)
            PYTEST_ARGS="$PYTEST_ARGS --cov=src/applications/lumos_simple --cov-report=html --cov-report=term"
            shift
            ;;
        -v|--verbose)
            # Already default
            shift
            ;;
        *)
            # Assume it's a test file or path
            PYTEST_ARGS="$PYTEST_ARGS $1"
            shift
            ;;
    esac
done

# Run tests
echo -e "${GREEN}Running tests...${NC}"
echo "Command: pytest $SCRIPT_DIR $PYTEST_ARGS"
echo ""

cd "$REPO_ROOT"
pytest "$SCRIPT_DIR" $PYTEST_ARGS

exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ All tests passed!${NC}"
else
    echo ""
    echo -e "${RED}✗ Some tests failed${NC}"
fi

exit $exit_code
