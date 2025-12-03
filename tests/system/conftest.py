"""
Pytest configuration and fixtures for Lumos system tests
"""
import os
import sys
import pytest
import subprocess
import tempfile
import shutil
from pathlib import Path


@pytest.fixture
def lumos_root():
    """Get the Lumos repository root directory"""
    # This file is in tests/system/, so go up two levels
    return Path(__file__).parent.parent.parent.absolute()


@pytest.fixture
def lumos_binary(lumos_root):
    """Get path to the lumos_dev binary"""
    binary_path = lumos_root / "build" / "src" / "applications" / "lumos_simple" / "lumos_dev"

    if not binary_path.exists():
        pytest.skip(f"lumos_dev binary not found at {binary_path}. Please build the project first.")

    if not os.access(binary_path, os.X_OK):
        pytest.skip(f"lumos_dev binary is not executable: {binary_path}")

    return binary_path


@pytest.fixture
def temp_project_dir(tmp_path):
    """Create a temporary directory for project testing"""
    project_dir = tmp_path / "test_project"
    project_dir.mkdir()

    # Change to the project directory
    original_cwd = os.getcwd()
    os.chdir(project_dir)

    yield project_dir

    # Restore original directory
    os.chdir(original_cwd)


@pytest.fixture
def run_lumos(lumos_binary):
    """Fixture that provides a function to run lumos commands"""
    def _run(args, input_text=None, check=True, timeout=30):
        """
        Run a lumos command

        Args:
            args: List of command arguments (e.g., ['init'])
            input_text: Optional input to provide to stdin
            check: Whether to check return code (raise on non-zero)
            timeout: Command timeout in seconds

        Returns:
            subprocess.CompletedProcess with stdout, stderr, returncode
        """
        cmd = [str(lumos_binary)] + args

        result = subprocess.run(
            cmd,
            input=input_text,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False  # We'll handle checking manually
        )

        if check and result.returncode != 0:
            print(f"Command failed: {' '.join(cmd)}")
            print(f"stdout: {result.stdout}")
            print(f"stderr: {result.stderr}")
            raise subprocess.CalledProcessError(
                result.returncode, cmd, result.stdout, result.stderr
            )

        return result

    return _run


@pytest.fixture
def clean_project_dir(temp_project_dir):
    """Ensure project directory is clean before each test"""
    # Remove any files that might have been created
    for item in temp_project_dir.iterdir():
        if item.is_file():
            item.unlink()
        elif item.is_dir():
            shutil.rmtree(item)

    return temp_project_dir
