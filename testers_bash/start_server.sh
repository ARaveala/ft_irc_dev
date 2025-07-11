#!/bin/bash
# Usage: ./start_server.sh <port> <password>
echo "Starting server..."
mkdir -p log_comp # Create log directory if it doesn't exist
mkdir -p log_comp/pids # Create pids directory if it doesn't exist
mkdir -p log_comp/client_inputs # Create client inputs directory if it doesn't exist
mkdir -p log_comp/active_clients # Create active clients directory if it doesn't exist
mkdir -p log_comp/command_queue # Create command queue directory if it doesn't exist

PORT="$1"
PASSWORD="$2"
EXEC="$(dirname "$(realpath "$0")")/../v1.2/ft_irc" # Get executable path relative to this script
SERVER_PID_TXT="server_pid.log"
VALGRIND_LOG="log_comp/valgrind_server.log"
SERVER_OUTPUT="log_comp/server_output.log"

> "$SERVER_OUTPUT"  # Empty the file
> "$SERVER_PID_TXT"  # Empty the file
> "$VALGRIND_LOG"  # Empty the Valgrind log file	

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes --verbose \
--log-file="$VALGRIND_LOG" "$EXEC" "$PORT" "$PASSWORD" > $SERVER_OUTPUT 2>&1  &


SERVER_PID=$!
echo "$SERVER_PID" > $SERVER_PID_TXT

sleep 2 # Wait for the server to start

# if out put is not server is listening on port
if ! grep -q "server is listening on port$PORT" "$SERVER_OUTPUT"; then
  echo "Error: Server failed to start on port $PORT"
  exit 1
else
  echo "Server started successfully on port $PORT"
fi
