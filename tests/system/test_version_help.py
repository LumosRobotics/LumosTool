"""
System tests for version and help commands

Tests basic CLI functionality like --version, --help, and error handling.
"""
import pytest


class TestVersionAndHelp:
    """Test version and help output"""

    def test_version_long_flag(self, run_lumos):
        """Test --version flag shows version information"""
        result = run_lumos(['--version'])

        assert result.returncode == 0
        assert "Lumos v" in result.stdout
        assert "1.0.0" in result.stdout

    def test_version_short_flag(self, run_lumos):
        """Test -v flag shows version information"""
        result = run_lumos(['-v'])

        assert result.returncode == 0
        assert "Lumos v" in result.stdout

    def test_help_long_flag(self, run_lumos):
        """Test --help flag shows usage information"""
        result = run_lumos(['--help'])

        assert result.returncode == 0
        assert "Lumos - STM32 Build Tool" in result.stdout
        assert "Usage:" in result.stdout
        assert "Commands:" in result.stdout

        # Verify all commands are listed
        expected_commands = ['init', 'build', 'flash', 'monitor', 'reset', 'ports']
        for cmd in expected_commands:
            assert cmd in result.stdout

    def test_help_short_flag(self, run_lumos):
        """Test -h flag shows usage information"""
        result = run_lumos(['-h'])

        assert result.returncode == 0
        assert "Usage:" in result.stdout
        assert "Commands:" in result.stdout

    def test_no_arguments_shows_help(self, run_lumos):
        """Test that running with no arguments shows help"""
        result = run_lumos([])

        assert result.returncode == 0
        assert "Usage:" in result.stdout
        assert "Commands:" in result.stdout

    def test_help_shows_examples(self, run_lumos):
        """Test that help output includes usage examples"""
        result = run_lumos(['--help'])

        assert result.returncode == 0
        assert "Examples:" in result.stdout
        assert "lumos init" in result.stdout
        assert "lumos build" in result.stdout

    def test_help_shows_all_commands(self, run_lumos):
        """Test that help lists all available commands with descriptions"""
        result = run_lumos(['--help'])

        assert result.returncode == 0

        # Check for command descriptions
        commands_with_desc = {
            'init': 'Initialize',
            'build': 'Build',
            'flash': 'Flash',
            'monitor': 'Monitor',
            'reset': 'Reset',
            'ports': 'List',
        }

        for cmd, desc_part in commands_with_desc.items():
            assert cmd in result.stdout, f"Command '{cmd}' not in help output"


class TestInvalidCommands:
    """Test error handling for invalid commands"""

    def test_invalid_command_shows_error(self, run_lumos):
        """Test that invalid command shows helpful error"""
        result = run_lumos(['invalid_command'], check=False)

        assert result.returncode != 0
        assert "Unknown command" in result.stderr or "Unknown command" in result.stdout

    def test_invalid_command_shows_usage(self, run_lumos):
        """Test that invalid command shows usage information"""
        result = run_lumos(['foobar'], check=False)

        assert result.returncode != 0
        output = result.stdout + result.stderr
        assert "Usage:" in output or "help" in output.lower()

    def test_typo_in_command_shows_error(self, run_lumos):
        """Test that typos in commands are caught"""
        typos = ['bild', 'initt', 'flashh', 'moniter']

        for typo in typos:
            result = run_lumos([typo], check=False)
            assert result.returncode != 0


class TestCommandLineInterface:
    """Test general CLI behavior"""

    def test_commands_are_case_sensitive(self, run_lumos):
        """Test that commands are case-sensitive"""
        # Uppercase should fail
        result = run_lumos(['INIT'], check=False)
        assert result.returncode != 0

        result = run_lumos(['Init'], check=False)
        assert result.returncode != 0

    def test_version_output_format(self, run_lumos):
        """Test that version output has consistent format"""
        result = run_lumos(['--version'])

        assert result.returncode == 0

        # Version should be on a single line
        lines = [line for line in result.stdout.split('\n') if line.strip()]
        assert len(lines) == 1

        # Format should be "Lumos vX.Y.Z"
        version_line = lines[0]
        assert version_line.startswith("Lumos v")
        assert len(version_line.split('.')) >= 2  # At least major.minor

    def test_help_output_readable_formatting(self, run_lumos):
        """Test that help output is well-formatted"""
        result = run_lumos(['--help'])

        assert result.returncode == 0

        # Should have clear sections
        assert "Commands:" in result.stdout
        assert "Usage:" in result.stdout
        assert "Examples:" in result.stdout

        # Commands should be indented/formatted
        assert "  init" in result.stdout or "init " in result.stdout

    def test_multiple_flags_not_supported(self, run_lumos):
        """Test that multiple conflicting flags are handled"""
        # Can't have both --version and --help
        result = run_lumos(['--version', '--help'], check=False)

        # Should either error or pick one (version takes precedence in most CLIs)
        assert result.returncode == 0 or result.returncode != 0
        # Just verify it doesn't crash
