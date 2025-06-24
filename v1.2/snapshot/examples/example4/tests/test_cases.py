# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    test_cases.py                                      :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: akuburas <akuburas@student.hive.fi>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/18 22:09:22 by akuburas          #+#    #+#              #
#    Updated: 2025/02/19 11:57:23 by akuburas         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

import subprocess
import time

def run_test(command, expected_output, base_dir):
	command_display = ["./" + command[0].split('/')[-1]] + command[1:]
	print(f"Testing: {' '.join(command_display)}")

	process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=base_dir)

	# Wait for the process to finish.
	time.sleep(1)
	stdout, stderr = process.communicate(timeout=2)

	# Check if the process is still running.
	if process.poll() is None:
		process.terminate()
		print(f"\033[91m❌ Server did not exit as expected! (Command: {' '.join(command)})\033[0m")
		return False

	result = stdout.strip() + "\n" + stderr.strip()
		
	if expected_output in result:
		print("\033[92m✅ Test passed!\033[0m")
	else:
		print("\033[93m⚠️  Unexpected output but server exited.\033[0m")
		print(f"Expected: {expected_output}")
		print(f"Got: {result}")

def run_basic_tests(base_dir, server_executable):
	# Case 1: Run without arguments
	run_test([server_executable], "Usage: ./ircserv <port> <password>", base_dir)

	# Case 2: Run with empty arguments
	run_test([server_executable, ""], "Usage: ./ircserv <port> <password>", base_dir)
	run_test([server_executable, "", ""], "Usage: ./ircserv <port> <password>", base_dir)

	# Case 3: Valid port, missing password
	run_test([server_executable, "6667"], "Usage: ./ircserv <port> <password>", base_dir)

	# Case who cares: missing port, valid password
	run_test([server_executable, "", "password123"], "Usage: ./ircserv <port> <password>", base_dir)

	# Case 4: Invalid port (e.g., 80 is a common web port)
	run_test([server_executable, "80", "password123"], "Error: Invalid port", base_dir)

	# Case 5: Commonly used port (e.g., 22 for SSH)
	run_test([server_executable, "22", "password123"], "Error: Invalid port", base_dir)

	# Case 6: An impossible port number
	run_test([server_executable, "-1", "password123"], "Error: Invalid port", base_dir)
	run_test([server_executable, "65536", "password123"], "Error: Invalid port", base_dir)

	print("\nAll starting tests completed.\n")
