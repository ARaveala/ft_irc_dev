#!/bin/bash

# Usage: ./force_exit_client.sh <session_name> <terminal>
SESSION_NAME="$1"
TERMINAL="${2:-tmux}"  # Defaults to 'tmux' if not specified
# Check if session name is provided
if [ -z "$SESSION_NAME" ]; then
  echo "Error: SESSION_NAME not provided"
  exit 1
fi

# Check if the session exists
if [ "$TERMINAL" == "tmux" ]; then
  if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
	echo "Killing tmux session '$SESSION_NAME'..."
	tmux kill-session -t "$SESSION_NAME"
  else
	echo "No tmux session found with name '$SESSION_NAME'."
  fi
elif [ "$TERMINAL" == "terminator" ]; then
  echo "Terminator does not support session management like tmux. Please close the window manually."
elif [ "$TERMINAL" == "screen" ]; then
  if screen -list | grep -q "$SESSION_NAME"; then
	echo "Killing screen session '$SESSION_NAME'..."
	screen -S "$SESSION_NAME" -X quit
  else
	echo "No screen session found with name '$SESSION_NAME'."
  fi
else
  echo "Unknown terminal option: $TERMINAL"
  exit 1
fi
echo "Session '$SESSION_NAME' has been terminated."
