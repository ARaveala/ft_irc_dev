# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    irssi_tests.py                                     :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: akuburas <akuburas@student.hive.fi>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/18 22:09:17 by akuburas          #+#    #+#              #
#    Updated: 2025/03/12 09:44:31 by akuburas         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

import subprocess
import random
import time
import threading
import os
import signal
import sys
import io
import select

# Approved lists
APPROVED_NICKNAMES = ["Alice", "Bob", "Charlie", "Dave", "Eve",
					"Frank", "Grace", "Heidi", "Ivan", "Judy"]
APPROVED_MESSAGES = [
	"Hello everyone!",
	"How's it going?",
	"Anyone here?",
	"What's up?",
	"I love this channel!",
	"Random message here.",
	"Testing, testing...",
	"This is fun!",
	"Just passing by.",
	"Goodbye for now."
]

APPROVED_CHANNEL_NAMES = ["#general", "#random", "#test", "#chat", "#fun"]

# tmux simulation parameters
SESSION_NAME = "irssi_sim"
NUM_CLIENTS = 5
running_threads_lock = threading.Lock()
running_threads = True  # Global flag to stop threads
user_input_recieved_lock = threading.Lock()
user_input_recieved = False
SESSION_DURATION = 1 * 60  # 1 minute



def create_tmux_session(session_name):
	# Kill any existing session with the same name
	subprocess.run(["tmux", "kill-session", "-t", session_name], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
	# Create a new session
	result = subprocess.run(["tmux", "new-session", "-d", "-s", session_name])
	if result.returncode != 0:
		print(f"[ERROR] Failed to create tmux session '{session_name}'.")
		return False
	print(f"[INFO] Created tmux session '{session_name}'.")
	return True


def create_tmux_window(session, window_name, command):
	result = subprocess.run(["tmux", "new-window", "-t", session, "-n", window_name, command])
	if result.returncode != 0:
		print(f"[ERROR] Failed to create tmux window '{window_name}' with command: {command}")
		return False
	print(f"[INFO] Created tmux window '{window_name}' with command: {command}")
	return True


def send_tmux_command(target, command_str):
	result = subprocess.run(["tmux", "send-keys", "-t", target, command_str, "C-m"])
	if result.returncode != 0:
		print(f"[ERROR] Failed to send command to tmux target '{target}': {command_str}")


def start_irssi_client(nickname, server_port, server_password):
	"""Starts an irssi client in a tmux window."""
	irssi_command = f"irssi -c 127.0.0.1 -p {server_port} -w {server_password} -n {nickname}"
	if not create_tmux_window(SESSION_NAME, nickname, irssi_command):
		print(f"[ERROR] Failed to start irssi client '{nickname}'.")
	time.sleep(1)  # Give the client time to start
	return nickname


def message_sender_thread(client):
	"""Thread function to send messages periodically."""
	global running_threads
	target = f"{SESSION_NAME}:{client}"
	
	while running_threads:
		msg = random.choice(APPROVED_MESSAGES)
		send_tmux_command(target, msg)
		time.sleep(random.uniform(0.5, 3))  # Random delay between messages


def check_client_connection(client):
	"""Check if the irssi client is still connected to the server."""
	target = f"{SESSION_NAME}:{client}"
	send_tmux_command(target, "/who")
	send_tmux_command(target, "/window 1")  # Switch to the first window
	time.sleep(1)  # Give time for the command to execute
	result = subprocess.run(["tmux", "capture-pane", "-t", target, "-p"], capture_output=True, text=True)
	output = result.stdout
	send_tmux_command(target, "/window last")  # Switch back to the last window
	return "Not connected to server" not in output


def check_server_status(server_pid):
	"""Check if the server process is still running."""
	try:
		os.kill(int(server_pid), 0)
	except OSError:
		return False
	return True


def monitoring_thread(clients, server_pid):
	"""Thread function to monitor the status of clients and the server."""
	global running_threads
	while True:
		with running_threads_lock:
			if not running_threads:
				break
		for client in clients:
			if not check_client_connection(client):
				print(f"[ERROR] Client {client} disconnected.")
				with running_threads_lock:
					running_threads = False
				break
		if not check_server_status(server_pid):
			print("[ERROR] Server crashed.")
			with running_threads_lock:
				running_threads = False
			break
		time.sleep(5)  # Check every 5 seconds


def check_user_input():
	"""Check if the user has pressed Enter to stop the simulation."""
	global user_input_recieved
	while True:
		if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
			user_input = sys.stdin.readline().strip()
			if user_input == "":
				break
		with user_input_recieved_lock:
			if user_input_recieved:
				break
	with user_input_recieved_lock:
		user_input_recieved = True

def timer(duration):
	global running_threads
	start_time = time.time()
	end_time = start_time + duration
	while time.time() < end_time:
		remain_time = int(end_time - time.time())
		minutes, seconds = divmod(remain_time, 60)
		hours, minutes = divmod(minutes, 60)
		timer_display = f"{hours:02d}:{minutes:02d}:{seconds:02d}"
		sys.stdout.write(f"\r[INFO] Simulation will stop in {timer_display}...")
		sys.stdout.flush()
		time.sleep(1)
	with running_threads_lock:
		running_threads = False


def start_simulation(server_port, server_password, server_pid):
	"""
	Launches an interactive simulation using threads.
	
	- Clients connect to the server at 127.0.0.1 using the given
	server_port and server_password.
	- A random channel name is generated.
	- NUM_CLIENTS irssi clients (each in their own tmux window) join that channel.
	- Each client starts sending random messages in a separate thread.
	
	The simulation runs until the user presses Enter or a client disconnects.
	"""
	global running_threads
	
	# Generate a random channel name
	channel_name = f"#{random.choice(['general', 'random', 'chat', 'test'])}{random.randint(1, 100)}"
	
	# Create a new tmux session for the simulation
	if not create_tmux_session(SESSION_NAME):
		return
	
	# Launch clients with unique random nicknames
	nicknames = random.sample(APPROVED_NICKNAMES, NUM_CLIENTS)
	clients = []
	
	for nick in nicknames:
		start_irssi_client(nick, server_port, server_password)
		clients.append(nick)
	
	print(f"[INFO] Simulation started with {NUM_CLIENTS} clients on channel {channel_name}.")
	print(f"[INFO] Server running on port {server_port} with password '{server_password}'.")
	print("\n[INFO] All clients have started. They should join the channel and begin messaging.")
	print("[INFO] Press Enter here to stop the simulation...\n")
	
	# Have each client join the channel
	for client in clients:
		send_tmux_command(f"{SESSION_NAME}:{client}", f"/join {channel_name}")
		time.sleep(0.5)
	
	# Start messaging threads
	threads = []
	
	for client in clients:
		thread = threading.Thread(target=message_sender_thread, args=(client,))
		thread.start()
		threads.append(thread)
	
	# Start monitoring thread
	time.sleep(1)  # Wait for clients to connect
	monitor_thread = threading.Thread(target=monitoring_thread, args=(clients, server_pid))
	monitor_thread.start()
	
	# Wait for user input to stop simulation
	print("To check the simulation, you can attach to the tmux session:")
	print(f"  tmux attach-session -t {SESSION_NAME}")
	print("To explore the windows, use Ctrl+b followed by a number (0-9).")
	print("To detach from the session, use Ctrl+b followed by d.")
	print("\n")
	print("To attach to the server session, use:")
	print(f"  tmux attach-session -t ircserv_sim\n")
	check_user_input_thread = threading.Thread(target=check_user_input)
	check_user_input_thread.start()
	timer_thread = threading.Thread(target=timer, args=(SESSION_DURATION,))
	timer_thread.start()
	while True:
		with user_input_recieved_lock:
			if user_input_recieved:
				break
		with running_threads_lock:
			if not running_threads:
				print("[INFO] Stopping simulation...")
				print("[INFO] Press Enter to exit.")
				break
		time.sleep(1)
	
	# Stop threads
	for thread in threads:
		thread.join()
	monitor_thread.join()
	check_user_input_thread.join()
	timer_thread.join()
	
	# Kill the tmux session (closing all simulation windows)
	subprocess.run(["tmux", "kill-session", "-t", SESSION_NAME])
	print("[INFO] Simulation stopped and tmux session killed.")


# Example usage:
# server_pid = <PID of the server process>
# start_simulation(6667, "mypassword", server_pid)


# Example usage:
# server_pid = <PID of the server process>
# start_simulation(6667, "mypassword", server_pid)