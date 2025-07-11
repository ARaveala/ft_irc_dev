#!/bin/bash

# Usage: ./connect_client.sh <port> <password> <nickname>
PORT="$1"
PASSWORD="$2"
NICKNAME="$3"
#SESSIONNAME="$4"  # Defaults to 'default_session' if not specified
#TERMINAL="${5:-tmux}"  # Defaults to 'tmux' if not specified

# Server hostname or IP, localhost for testing
SERVER="localhost"
CLIENT_LOGS="log_comp/client_logs/${NICKNAME}.log"
PIPE="log_comp/client_inputs/${NICKNAME}.in"
QUEUE="log_comp/command_queue/${NICKNAME}.txt"
# If no session name is provided, warn and exit
mkdir -p "$(dirname "$PIPE")" "$(dirname "$QUEUE")" "$(dirname "$CLIENT_LOGS")"

# Clean or recreate pipe BEFORE launching anything
if [ -e "$PIPE" ]; then
  if [ ! -p "$PIPE" ]; then
    echo " '$PIPE' exists but is not a pipe — removing"
    rm -f "$PIPE"
  else
    echo " Removing existing pipe for $NICKNAME"
    rm -f "$PIPE"
  fi
fi

# Now create the fresh pipe
mkfifo "$PIPE"
touch "$QUEUE"
: > "$QUEUE" # empty histroical data
echo "Created pipe for $NICKNAME"

# Launch queue feeder ONLY after pipe is safely created
tail -n0 -f "$QUEUE" > "$PIPE" &
echo $! > "log_comp/pids/${NICKNAME}_queue.pid"


# run netcat to connect to the server
nc "$SERVER" "$PORT" < "$PIPE" > "$CLIENT_LOGS" &
echo $! > "log_comp/pids/${NICKNAME}.pid"
sleep 0.2

# Send registration
printf "CAP LS\r\nPASS $PASSWORD\r\nNICK $NICKNAME\r\nUSER $NICKNAME 0 * :Test Bot\r\n" >> "$QUEUE"

COUNT=0
TIMEOUT=10  # Timeout in seconds
while ! grep -q "End of /MOTD" "$CLIENT_LOGS"; do

  echo "Waiting for MOTD for $NICKNAME..."
  sleep 1
  COUNT=$((COUNT + 1))
  if [ "$COUNT" -ge "$TIMEOUT" ]; then
	echo "Warning: Registartion was not deemed complete  $TIMEOUT seconds for $NICKNAME"
	break
  fi
done
COUNT=0
while ! grep -q "Server ready, you may give input" "$CLIENT_LOGS"; do
  printf "PING :localhost\r\n" | tee -a log_comp/client_logs/${NICKNAME}in.log  >> "$QUEUE"
  sleep 1
  COUNT=$((COUNT + 1))
  if [ "$COUNT" -ge "$TIMEOUT" ]; then
    echo "Warning: Server did not become ready within $TIMEOUT seconds for $NICKNAME"
    break
  fi
done

echo "Server is ready for $NICKNAME proceeding with next steps."



#when we may want irssi to connect to the server
#if [ -z "$SESSIONNAME" ]; then
#  echo "Error: SESSIONNAME not provided"
#  exit 1
#fi
#
## Check if tmux session already exists
#if [ "$TERMINAL" == "tmux" ]; then
#  if tmux has-session -t "$SESSIONNAME" 2>/dev/null; then
#    echo "Session '$SESSIONNAME' already exists. Skipping client launch."
#    exit 1
#  fi
#fi

# Check if the required parameters are provided
#if [ "$TERMINAL" == "tmux" ]; then
#  tmux new-session -d -s "$SESSIONNAME" "irssi -c $SERVER -p $PORT -w $PASSWORD -n $NICKNAME"
#elif [ "$TERMINAL" == "terminator" ]; then
#  terminator -e "irssi -c $SERVER -p $PORT -w $PASSWORD -n $NICKNAME" &
#elif [ "$TERMINAL" == "screen" ]; then
#  screen -dmS "$SESSIONNAME" irssi -c "$SERVER" -p "$PORT" -w "$PASSWORD" -n "$NICKNAME"
#else
#  echo "Unknown terminal option: $TERMINAL"
#  exit 1
#fi



