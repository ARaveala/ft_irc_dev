# tests/run_all_tests.py
import unittest
import os
import sys
import subprocess
import time
import importlib
import io # Ensure this is imported

# Import configurations and base class from utils
from irc_tester_utils import SERVER_PATH, SERVER_PORT, SERVER_PASSWORD


# --- Directory Definitions ---
TESTS_DIR = os.path.dirname(os.path.abspath(__file__)) # This script is in 'tests'
OUTPUTS_DIR = os.path.join(TESTS_DIR, '..', 'outputs') # Outputs will be in a sibling 'outputs' directory

# --- Global Server Management Functions ---
_global_server_process = None
_server_stdout_capture = io.BytesIO() # To capture server's stdout/stderr
_server_stderr_capture = io.BytesIO() # throughout its lifetime

def start_global_server():
    global _global_server_process
    global _server_stdout_capture, _server_stderr_capture # Declare globals to reset them

    # Re-initialize capture buffers for a clean start
    _server_stdout_capture = io.BytesIO()
    _server_stderr_capture = io.BytesIO()

    print(f"\nStarting global server: {SERVER_PATH} {SERVER_PORT} {SERVER_PASSWORD}")
    _global_server_process = subprocess.Popen(
        [SERVER_PATH, str(SERVER_PORT), SERVER_PASSWORD],
        stdout=subprocess.PIPE, # Correctly use PIPE to capture output
        stderr=subprocess.PIPE,  # Correctly use PIPE to capture output
        text=True, # Decode stdout/stderr as text for readline
        bufsize=1, # Line-buffered output
        universal_newlines=True # Handle different line endings
    )
    
    # Actively wait for the "server is listening" message
    server_ready = False
    start_time = time.time()
    timeout = 10 # seconds to wait for server to be ready (increased timeout)
    while time.time() - start_time < timeout:
        line = _global_server_process.stdout.readline()
        if line:
            # Write to BytesIO for later full capture, but also print to console for live feedback
            _server_stdout_capture.write(line.encode('utf-8'))
            print(f"[Server Startup Log]: {line.strip()}")
            if "server is listening" in line:
                server_ready = True
                break
        # Check if server process has terminated prematurely
        if _global_server_process.poll() is not None:
            stdout_data_after_crash = _global_server_process.stdout.read()
            stderr_data_after_crash = _global_server_process.stderr.read()
            _server_stdout_capture.write(stdout_data_after_crash.encode('utf-8'))
            _server_stderr_capture.write(stderr_data_after_crash.encode('utf-8'))
            raise Exception(f"Global server exited prematurely during startup. Check server's stderr/logs. Exit code: {_global_server_process.returncode}\nCaptured stdout:\n{_server_stdout_capture.getvalue().decode(errors='ignore')}\nCaptured stderr:\n{_server_stderr_capture.getvalue().decode(errors='ignore')}")
        time.sleep(0.05) # Small delay to prevent busy-waiting

    if not server_ready:
        # If server didn't indicate listening within timeout, check if it's still alive or crashed later
        if _global_server_process.poll() is not None:
            stdout_data_after_timeout = _global_server_process.stdout.read()
            stderr_data_after_timeout = _global_server_process.stderr.read()
            _server_stdout_capture.write(stdout_data_after_timeout.encode('utf-8'))
            _server_stderr_capture.write(stderr_data_after_timeout.encode('utf-8'))
            raise Exception(f"Global server did not indicate 'server is listening' within {timeout} seconds and exited. Exit code: {_global_server_process.returncode}\nCaptured stdout:\n{_server_stdout_capture.getvalue().decode(errors='ignore')}\nCaptured stderr:\n{_server_stderr_capture.getvalue().decode(errors='ignore')}")
        else:
            raise Exception(f"Global server did not indicate 'server is listening' within {timeout} seconds but is still running. Investigate server logs.")
    else:
        print("Global server started successfully.")
        time.sleep(0.5) # Give it a brief moment after readiness for final setup

def stop_global_server():
    global _global_server_process
    if _global_server_process is None:
        return

    print(f"\nStopping global server (PID: {_global_server_process.pid})")
    
    # Terminate the process
    _global_server_process.terminate()
    
    try:
        # Use communicate() to get any remaining output from pipes and wait for termination.
        stdout_data, stderr_data = _global_server_process.communicate(timeout=5) # Increased timeout
        _server_stdout_capture.write(stdout_data.encode('utf-8')) # Append any remaining stdout
        _server_stderr_capture.write(stderr_data.encode('utf-8')) # Append any remaining stderr
    except subprocess.TimeoutExpired:
        print("Global server did not terminate gracefully, killing...")
        _global_server_process.kill() # Force kill if terminate timed out
        # Call communicate again after kill to ensure all pipes are drained and process is truly gone
        stdout_data, stderr_data = _global_server_process.communicate() 
        _server_stdout_capture.write(stdout_data.encode('utf-8'))
        _server_stderr_capture.write(stderr_data.encode('utf-8'))
        _global_server_process.wait() # Ensure process has fully exited
    
    # Print all captured output from the BytesIO buffers
    stdout_content = _server_stdout_capture.getvalue().decode(errors='ignore')
    stderr_content = _server_stderr_capture.getvalue().decode(errors='ignore')
    print(f"Global server stdout:\n{stdout_content}")
    print(f"Global server stderr:\n{stderr_content}")
    _global_server_process = None
    print("Global server stopped.")


# --- Test Suite Runner ---
def run_tests_and_save_output_per_file():
    # Create outputs directory if it doesn't exist
    os.makedirs(OUTPUTS_DIR, exist_ok=True)
    print(f"Ensured '{OUTPUTS_DIR}' directory exists for test outputs.")

    # Start the server once for the entire test run
    try:
        start_global_server()
    except Exception as e:
        print(f"FATAL ERROR: Could not start the global server. Aborting tests. {e}")
        return 1

    overall_success = True
    
    print("="*30)
    print("Starting IRC Server Test Suite (Per-File Output)")
    print("="*30)

    # Get a list of all test files from the TESTS_DIR
    all_test_files_full_paths = [os.path.join(TESTS_DIR, f) for f in os.listdir(TESTS_DIR) if f.startswith('test_') and f.endswith('.py')]
    
    # Add the TESTS_DIR to sys.path so modules can be imported directly by name
    if TESTS_DIR not in sys.path:
        sys.path.insert(0, TESTS_DIR)

    try:
        for test_file_path in sorted(all_test_files_full_paths):
            test_file_name = os.path.basename(test_file_path) # e.g., 'test_basic_connection.py'
            test_module_name = os.path.splitext(test_file_name)[0] # e.g., 'test_basic_connection'
            output_filename = os.path.join(OUTPUTS_DIR, f"{test_module_name}_output.txt")

            print(f"\nRunning tests from '{test_file_name}' and saving output to '{output_filename}'...")
            
            # Temporarily redirect stdout and stderr to the output file
            original_stdout = sys.stdout
            original_stderr = sys.stderr
            
            with open(output_filename, 'w') as f_out:
                sys.stdout = f_out
                sys.stderr = f_out # Redirect stderr as well for tracebacks

                try:
                    # Import the test module dynamically
                    module = importlib.import_module(test_module_name)
                    # Reload the module to ensure fresh state if it was imported before in a previous loop
                    importlib.reload(module) 
                    
                    suite = unittest.TestLoader().loadTestsFromModule(module)
                    
                    runner = unittest.TextTestRunner(verbosity=2, stream=sys.stdout)
                    result = runner.run(suite)
                    if not result.wasSuccessful():
                        overall_success = False

                except Exception as e:
                    print(f"Error running tests from {test_file_name}: {e}")
                    overall_success = False
                finally:
                    # ALWAYS restore stdout/stderr
                    sys.stdout = original_stdout
                    sys.stderr = original_stderr
            
            # Print a summary to the main console after each test file completes
            with open(output_filename, 'r') as f_in:
                print(f"--- Summary for '{test_module_name}' ---")
                summary_lines = f_in.readlines()[-6:] # Get a few lines to ensure summary is captured
                print("".join(summary_lines).strip())
                print(f"Full details in '{output_filename}'\n")

    finally:
        # Ensure the server is stopped regardless of test outcomes for the entire suite
        stop_global_server()
        # Remove TESTS_DIR from sys.path if it was added
        if TESTS_DIR in sys.path:
            sys.path.remove(TESTS_DIR)

    print("="*30)
    print("Test Suite Finished")
    print("="*30)
    
    if overall_success:
        print("\nALL TEST FILES PASSED SUCCESSFULLY!")
        return 0
    else:
        print("\nSOME TEST FILES FAILED OR HAD ERRORS! Check individual output files in the 'outputs/' directory.")
        return 1

if __name__ == '__main__':
    sys.exit(run_tests_and_save_output_per_file())
