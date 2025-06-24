# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    script.py                                          :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: akuburas <akuburas@student.hive.fi>        +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/02/10 13:38:42 by akuburas          #+#    #+#              #
#    Updated: 2025/02/17 11:39:32 by akuburas         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

import subprocess
import sys
import time

# Name of the tmux session to create/use.
SESSION_NAME = "irssi_session"

def create_tmux_session(session_name):
    """Creates a new tmux session in detached mode."""
    # This will fail if the session already exists.
    subprocess.run(["tmux", "new-session", "-d", "-s", session_name])

def create_tmux_window(session, window_name, command):
    """
    Creates a new tmux window in the given session, running the specified command.
    """
    subprocess.run(["tmux", "new-window", "-t", session, "-n", window_name, command])

def open_irssi_tmux(nickname, ip_address, port, password):
    """
    Launches irssi in a new tmux window with a name based on the nickname.
    """
    window_name = nickname
    # Build the irssi command with provided parameters.
    irssi_command = f"irssi -c {ip_address} -p {port} -w {password} -n {nickname}"
    create_tmux_window(SESSION_NAME, window_name, irssi_command)
    return window_name

def mode1(ip_address, port, password):
    base_nickname = input("Enter base nickname: ")
    count = int(input("Enter number of clients: "))
    windows = [open_irssi_tmux(f"{base_nickname}{i+1}", ip_address, port, password)
               for i in range(count)]
    return windows

def mode2(ip_address, port, password):
    nicknames = input("Enter nicknames separated by spaces: ").split()
    windows = [open_irssi_tmux(nickname, ip_address, port, password)
               for nickname in nicknames]
    return windows

def close_irssi(windows):
    """
    Sends the /exit command to each irssi client running in tmux,
    then kills the tmux session.
    """
    for win in windows:
        # Send "/exit" command followed by Enter (C-m) to the tmux window.
        subprocess.run(["tmux", "send-keys", "-t", f"{SESSION_NAME}:{win}", "/exit", "C-m"])
    # Give processes a moment to exit cleanly.
    time.sleep(1)
    # Kill the entire tmux session, which closes all windows.
    subprocess.run(["tmux", "kill-session", "-t", SESSION_NAME])

def main():
    ip_address = input("Enter IRC server IP address: ")
    port = input("Enter port: ")
    password = input("Enter password: ")
    
    # Create the tmux session.
    create_tmux_session(SESSION_NAME)
    
    mode = input("Select mode (1 or 2): ")
    windows = []
    
    if mode == "1":
        windows = mode1(ip_address, port, password)
    elif mode == "2":
        windows = mode2(ip_address, port, password)
    else:
        print("Invalid mode selected.")
        sys.exit(1)
    
    print("\nAll clients started in tmux windows within the session 'irssi_session'.")
    print("You can attach to the session with: tmux attach -t irssi_session")
    input("\nPress Enter when you want to close all clients...")
    close_irssi(windows)
    print("All clients closed.")

if __name__ == "__main__":
    main()

