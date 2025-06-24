#!/bin/bash

# This script launches the IRC server test suite.
# It assumes you are running it from the root directory of your project,
# where the 'tests/' and 'outputs/' directories are located.

echo "--- Starting IRC Test Suite ---"

# Execute the Python test runner script.
# The Python script 'run_all_tests.py' is located in the 'tests/' directory.
# Any arguments passed to this bash script will be forwarded to the Python script.
python3 tests/run_all_tests.py "$@"

# Check the exit code of the Python script
# (0 means success, non-zero means failure)
if [ $? -eq 0 ]; then
    echo "--- All IRC tests passed successfully! ---"
else
    echo "--- Some IRC tests failed. Check the 'outputs/' directory for detailed logs. ---"
fi

echo "--- IRC Test Suite Finished ---"