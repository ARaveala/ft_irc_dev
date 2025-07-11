#!/bin/bash

# Example: ./load_test.sh 6667 secretpass 50
PORT="$1"
PASSWORD="$2"
TERMINAL="${3:-tmux}"  # Defaults to 'tmux' if not specified

SERVER_PID_TXT="server_pid.txt"
NUM_CLIENTS="3"
#SESSION_NAME="sesh_"  # Defaults to 'default_session' if not specified
ACTIVE_CLIENTS_FILE="log_comp/active_clients.txt"
#SESSION_FILE="log_comp/active_sessions.txt"
#> "$SESSION_FILE"  # Empty the file
> "$ACTIVE_CLIENTS_FILE"  # Empty the file
if ./start_server.sh "$PORT" "$PASSWORD"; then
	echo "Server started successfully on port $PORT"
else
	echo "Error M_load_test: Server failed to start on port $PORT"
	exit 1
fi
echo "Starting load test with $NUM_CLIENTS clients on port $PORT"
for i in $(seq 1 "$NUM_CLIENTS"); do
    NICK="bot_$i"
	#SESSION_NAME="sesh_${NICK}"
	echo "$NICK" >> "$ACTIVE_CLIENTS_FILE"
	if ! ./connect_client.sh "$PORT" "$PASSWORD" "$NICK"; then
	  echo "Skipping next test due to failure, number of clients reached: $i"
	fi
    sleep 0.1  # Small delay to avoid flooding
done

echo "SUCCESS: Started $NUM_CLIENTS clients on port $PORT"
#sleep 10  # Wait for clients to connect
#echo "To stop the test, run: tmux kill-session -t $SESSION_NAME"

while read -r NICK; do
	./send_quit.sh "$NICK"
	#sleep 2
	#./send_quit.sh "$NICK"

done < "$ACTIVE_CLIENTS_FILE"	

#for i in $(seq 1 "$NUM_CLIENTS"); do
#	NICK="bot_$i"
#	./send_quit.sh "$NICK"
#done
echo "All clients have been sent the quit command killing server with pid."
./kill_pids.sh
echo "All clients processes have been terminated."

kill "$(cat server_pid.log)"
#while read SESSION; do
#  ./send_quit.sh "$SESSION" "$TERMINAL"
#done < "$SESSION_FILE"
#kill "$(cat sniff_pid.log)"
rm -f log_comp/client_inputs/bot_1.in
rm -f log_comp/client_inputs/bot_2.in
rm -f log_comp/client_inputs/bot_3.in