"""
System tests for 'lumos init' command

These tests verify the initialization workflow, file generation,
and user interaction for creating new Lumos projects.
"""
import os
import yaml
from pathlib import Path
import pytest


class TestLumosInit:
    """Test suite for lumos init command"""

    def test_init_creates_cpp_project_with_defaults(self, temp_project_dir, run_lumos):
        """Test that init creates a C++ project when using default options"""
        # Simulate user selecting default options (C++ and LumosBrain)
        # Input: press Enter twice to accept defaults
        result = run_lumos(['init'], input_text="\n\n")

        # Check command succeeded
        assert result.returncode == 0
        assert "✓ Project initialized successfully!" in result.stdout

        # Verify files were created
        assert (temp_project_dir / "main.cpp").exists()
        assert (temp_project_dir / "project.yaml").exists()
        assert (temp_project_dir / "README.md").exists()

        # Verify main.cpp contains expected content
        main_content = (temp_project_dir / "main.cpp").read_text()
        assert "void setup()" in main_content
        assert "void loop()" in main_content

        # Verify project.yaml has correct configuration
        with open(temp_project_dir / "project.yaml") as f:
            config = yaml.safe_load(f)
            assert config['board'] == 'LumosBrain'
            assert 'main.cpp' in config['sources']

    def test_init_creates_c_project(self, temp_project_dir, run_lumos):
        """Test that init creates a C project when C is selected"""
        # Select C language (option 2)
        result = run_lumos(['init'], input_text="\n2\n")

        assert result.returncode == 0

        # Verify C file was created
        assert (temp_project_dir / "main.c").exists()
        assert not (temp_project_dir / "main.cpp").exists()

        # Verify main.c contains expected content
        main_content = (temp_project_dir / "main.c").read_text()
        assert "void setup(void)" in main_content
        assert "void loop(void)" in main_content

        # Verify project.yaml references main.c
        with open(temp_project_dir / "project.yaml") as f:
            config = yaml.safe_load(f)
            assert 'main.c' in config['sources']

    def test_init_with_specific_board(self, temp_project_dir, run_lumos):
        """Test init with different board selection"""
        # Select LumosMiniBrain (option 2)
        result = run_lumos(['init'], input_text="2\n\n")

        assert result.returncode == 0

        # Verify board is set correctly in project.yaml
        with open(temp_project_dir / "project.yaml") as f:
            config = yaml.safe_load(f)
            assert config['board'] == 'LumosMiniBrain'

    def test_init_already_initialized_creates_only_missing_main(self, temp_project_dir, run_lumos):
        """Test that re-running init when project.yaml exists only creates missing main file"""
        # First initialization
        run_lumos(['init'], input_text="\n\n")

        # Remove main.cpp but keep project.yaml
        (temp_project_dir / "main.cpp").unlink()

        # Run init again - should only prompt for language
        result = run_lumos(['init'], input_text="\n")

        assert result.returncode == 0
        assert "project.yaml already exists" in result.stdout
        assert "✓ Main file created successfully!" in result.stdout

        # Verify only main.cpp was recreated
        assert (temp_project_dir / "main.cpp").exists()

    def test_init_fully_initialized_no_changes(self, temp_project_dir, run_lumos):
        """Test that init does nothing when project is fully initialized"""
        # First initialization
        run_lumos(['init'], input_text="\n\n")

        # Run init again - should report everything exists
        result = run_lumos(['init'], input_text="")

        assert result.returncode == 0
        assert "project.yaml already exists" in result.stdout
        assert "Main file already exists" in result.stdout
        assert "Project is ready" in result.stdout

    def test_init_creates_readme_with_correct_content(self, temp_project_dir, run_lumos):
        """Test that README.md is created with project-specific content"""
        result = run_lumos(['init'], input_text="2\n2\n")  # MiniBrain, C

        assert result.returncode == 0

        readme_path = temp_project_dir / "README.md"
        assert readme_path.exists()

        readme_content = readme_path.read_text()
        assert "LumosMiniBrain" in readme_content
        assert "C" in readme_content
        assert "lumos build" in readme_content
        assert "main.c" in readme_content

    def test_init_creates_valid_yaml_structure(self, temp_project_dir, run_lumos):
        """Test that project.yaml has valid structure"""
        run_lumos(['init'], input_text="\n\n")

        yaml_path = temp_project_dir / "project.yaml"
        assert yaml_path.exists()

        with open(yaml_path) as f:
            config = yaml.safe_load(f)

        # Verify required fields exist
        assert 'sources' in config
        assert 'board' in config

        # Verify sources is a list
        assert isinstance(config['sources'], list)
        assert len(config['sources']) > 0

        # Verify board is a string
        assert isinstance(config['board'], str)

    def test_init_output_shows_next_steps(self, temp_project_dir, run_lumos):
        """Test that init output provides helpful next steps"""
        result = run_lumos(['init'], input_text="\n\n")

        assert result.returncode == 0

        # Check for helpful guidance
        assert "Next steps:" in result.stdout
        assert "lumos build" in result.stdout
        assert "lumos flash" in result.stdout

    def test_init_main_file_has_setup_and_loop(self, temp_project_dir, run_lumos):
        """Test that generated main file has setup() and loop() functions"""
        # Test C++
        run_lumos(['init'], input_text="\n\n")
        main_cpp = (temp_project_dir / "main.cpp").read_text()

        assert "void setup()" in main_cpp
        assert "void loop()" in main_cpp
        assert "// Initialize your application here" in main_cpp or \
               "Initialize your application here" in main_cpp

        # Clean up and test C
        (temp_project_dir / "main.cpp").unlink()
        (temp_project_dir / "project.yaml").unlink()
        (temp_project_dir / "README.md").unlink()

        run_lumos(['init'], input_text="\n2\n")  # Select C
        main_c = (temp_project_dir / "main.c").read_text()

        assert "void setup(void)" in main_c
        assert "void loop(void)" in main_c

    def test_init_project_yaml_includes_hal_modules_comment(self, temp_project_dir, run_lumos):
        """Test that project.yaml includes helpful HAL modules comment"""
        run_lumos(['init'], input_text="\n\n")

        yaml_content = (temp_project_dir / "project.yaml").read_text()

        # Check for HAL modules documentation
        assert "hal_modules:" in yaml_content
        assert "uart" in yaml_content
        assert "spi" in yaml_content
        assert "i2c" in yaml_content
        assert "auto-detected if not specified" in yaml_content

    def test_init_with_different_boards(self, temp_project_dir, run_lumos):
        """Test init with all available board options"""
        boards = [
            ("1", "LumosBrain"),
            ("2", "LumosMiniBrain"),
            ("3", "LumosEscMini")
        ]

        for board_choice, expected_board in boards:
            # Clean directory
            for item in temp_project_dir.iterdir():
                if item.is_file():
                    item.unlink()

            # Initialize with this board
            result = run_lumos(['init'], input_text=f"{board_choice}\n\n")

            assert result.returncode == 0

            # Verify board selection
            with open(temp_project_dir / "project.yaml") as f:
                config = yaml.safe_load(f)
                assert config['board'] == expected_board, \
                    f"Expected board {expected_board}, got {config['board']}"


class TestLumosInitErrorCases:
    """Test error handling in lumos init"""

    def test_init_handles_invalid_choice_gracefully(self, temp_project_dir, run_lumos):
        """Test that invalid choices fall back to defaults"""
        # Provide invalid input, should use defaults
        result = run_lumos(['init'], input_text="invalid\ninvalid\n")

        assert result.returncode == 0
        # Should still create files with defaults
        assert (temp_project_dir / "main.cpp").exists()
        assert (temp_project_dir / "project.yaml").exists()

    def test_init_shows_helpful_output(self, temp_project_dir, run_lumos):
        """Test that init provides clear user prompts"""
        result = run_lumos(['init'], input_text="\n\n")

        # Check for user-friendly prompts
        assert "Lumos Project Initialization" in result.stdout
        assert "Select target board:" in result.stdout
        assert "Select programming language:" in result.stdout
        assert "Creating project in:" in result.stdout
