import unittest
import subprocess
import time
import os
import sys

# Add the parent directory of irc_tester/tests to sys.path
# so we can import irc_tester_utils from ../irc_tester_utils.py
current_dir = os.path.dirname(os.path.abspath(__file__))
irc_tester_root = os.path.join(current_dir, '..')
if irc_tester_root not in sys.path:
    sys.path.insert(0, irc_tester_root)

from irc_tester_utils import SERVER_PATH, SERVER_PORT

class PasswordValidationTests(unittest.TestCase):

    def start_server_with_args(self, port, password):
        """
        Starts the IRC server with given port and password arguments,
        captures its output, and returns the exit code, stdout, and stderr.
        """
        command = [SERVER_PATH, str(port), password]
        print(f"\nAttempting to start server with: {' '.join(command)}")

        process = None
        try:
            process = subprocess.Popen(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True, # Decode stdout/stderr as text
                universal_newlines=True # Handle different line endings
            )

            # Give the server a brief moment to process arguments and potentially exit
            time.sleep(0.2) # Increased sleep slightly

            # Communicate() waits for process to terminate and returns all output
            stdout_data, stderr_data = process.communicate(timeout=5) # Max 5 seconds to run/exit

            return process.returncode, stdout_data, stderr_data
        except subprocess.TimeoutExpired:
            # If communicate times out, it means the server didn't exit as expected.
            print(f"Server process timed out. Killing PID {process.pid}...")
            process.kill()
            stdout_data, stderr_data = process.communicate() # Capture remaining output after kill
            return -1, stdout_data, stderr_data # Indicate a timeout/failure
        except Exception as e:
            print(f"Error starting server subprocess: {e}")
            return -1, "", str(e) # Indicate a general error
        finally:
            if process and process.poll() is None: # If process is still running
                process.terminate()
                process.wait(timeout=1) # Wait for it to exit
                if process.poll() is None:
                    process.kill() # Final resort
                print("Server process ensured to be terminated.")


    # --- Test Cases for Invalid Passwords ---

    def test_01_empty_password(self):
        print("\n--- Running test_01_empty_password ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "")
        self.assertNotEqual(return_code, 0, "Server should fail for empty password")
        self.assertIn("empty", stderr.lower(), "Stderr should mention 'empty' for empty password")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    def test_02_whitespace_only_password(self):
        print("\n--- Running test_02_whitespace_only_password ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "   ")
        self.assertNotEqual(return_code, 0, "Server should fail for whitespace-only password")
        self.assertIn("whitespace", stderr.lower(), "Stderr should mention 'whitespace' for whitespace-only password")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    def test_03_too_short_password(self):
        print("\n--- Running test_03_too_short_password ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "abc") # Length 3, min 4
        self.assertNotEqual(return_code, 0, "Server should fail for too short password")
        self.assertIn("at least", stderr.lower(), "Stderr should mention 'at least'")
        self.assertIn("characters long", stderr.lower(), "Stderr should mention 'characters long'")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    def test_04_too_long_password(self):
        print("\n--- Running test_04_too_long_password ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "ThisPasswordIsWayTooLongForYourTwelveCharacterLimit") # Length 50, max 12
        self.assertNotEqual(return_code, 0, "Server should fail for too long password")
        self.assertIn("not exceed", stderr.lower(), "Stderr should mention 'not exceed'")
        self.assertIn("characters", stderr.lower(), "Stderr should mention 'characters'")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    def test_05_password_missing_char_types_lowercase_only(self):
        print("\n--- Running test_05_password_missing_char_types_lowercase_only ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "onlyletters") # Length 11, but only 1 type
        self.assertNotEqual(return_code, 0, "Server should fail for password with only lowercase")
        self.assertIn("character types", stderr.lower(), "Stderr should mention 'character types'")
        self.assertIn("at least 3", stderr.lower(), "Stderr should mention 'at least 3'")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    def test_06_password_missing_char_types_digits_only(self):
        print("\n--- Running test_06_password_missing_char_types_digits_only ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "12345678") # Length 8, but only 1 type
        self.assertNotEqual(return_code, 0, "Server should fail for password with only digits")
        self.assertIn("character types", stderr.lower(), "Stderr should mention 'character types'")
        self.assertIn("at least 3", stderr.lower(), "Stderr should mention 'at least 3'")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    def test_07_password_with_internal_whitespace(self):
        print("\n--- Running test_07_password_with_internal_whitespace ---")
        return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "my secret!") # Length 10, internal space
        self.assertNotEqual(return_code, 0, "Server should fail for password with internal whitespace")
        self.assertIn("invalid whitespace", stderr.lower(), "Stderr should mention 'invalid whitespace'")
        print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")

    # --- Test Case for Valid Password ---

    # def test_08_valid_password(self):
    #     print("\n--- Running test_08_valid_password ---")
    #     return_code, stdout, stderr = self.start_server_with_args(SERVER_PORT, "Pass_123!") # L:9, lower, upper, digit, special (4 types)
    #     self.assertEqual(return_code, 0, "Server should start successfully for a valid password")
    #     self.assertNotIn("error", stderr.lower(), "Stderr should not contain 'error' for valid password")
    #     self.assertIn("server is listening", stdout.lower(), "Server stdout should indicate it's listening")
    #     # For valid passwords, you might want to kill the server process after confirming it started
    #     # and capture the remaining output if it keeps running in background
    #     print(f"STDOUT:\n{stdout}\nSTDERR:\n{stderr}")


if __name__ == '__main__':
    # When running this file directly, it will run all tests in this class
    unittest.main()