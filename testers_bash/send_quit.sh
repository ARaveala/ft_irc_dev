#!/bin/bash

# Usage: ./send_quit.sh <session_name> [terminal]
NICKNAME="$1"
PIPE="log_comp/client_inputs/${NICKNAME}.in"
TEST="log_comp/client_logs/${NICKNAME}in.log"
CLIENT_LOGS="log_comp/client_logs/${NICKNAME}.log"
QUEUE="log_comp/command_queue/${NICKNAME}.txt"

#SESSION_NAME="$1"
#TERMINAL="${2:-tmux}"
# Check if nickname is provided
if [ -z "$NICKNAME" ]; then
  echo "Error: NICKNAME not provided"
  exit 1
fi
PID_FILE="log_comp/pids/${NICKNAME}.pid"
PID=$(cat "$PID_FILE" 2>/dev/null)

if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
  echo ""
else
  echo "Netcat process for $NICKNAME is not running"
fi

if [ -p "$PIPE" ]; then
   echo -ne "QUIT\r\n" >> "$QUEUE"
   echo "Sent /quit to $NICKNAME"
else
   echo "No pipe found for $NICKNAME — skipping"
fi
#if tail -n 10 "$CLIENT_LOGS" | grep -q "QUIT"; then
	sleep 1
	kill "$(cat log_comp/pids/${NICKNAME}_queue.pid)" 2>/dev/null
	sleep 1
	kill "$(cat log_comp/pids/${NICKNAME}.pid)" 2>/dev/null
	echo "After sending /quit to $NICKNAME, checking if netcat is still running..."
#fi

if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
  echo " Netcat process for $NICKNAME is alive"
else
  echo "Netcat process for $NICKNAME is not running"
fi

sleep 1
if ! kill -0 "$(cat log_comp/pids/${NICKNAME}.pid)" 2>/dev/null; then
  echo "✅ Netcat exited after QUIT — server closed the connection"
else
  echo "⏳ Netcat still running — server may not have closed the socket"
fi

# irssi usage
#if [ -z "$SESSION_NAME" ]; then
#  echo "Error: SESSION_NAME not provided"
#  exit 1
#fi
#echo "Sending '/quit' command to session '$SESSION_NAME' in terminal '$TERMINAL'..."
#
#if [ "$TERMINAL" == "tmux" ]; then
#  if tmux has-session -t "$SESSION_NAME" 2>/dev/null; then
#    echo "Sending '/quit' to session '$SESSION_NAME'..."
#    tmux send-keys -t "$SESSION_NAME" "/quit" C-m
#  else
#    echo "No tmux session found with name '$SESSION_NAME'."
#    exit 1
#  fi
#else
#  echo "Terminal '$TERMINAL' not supported for scripted IRC commands."
#  exit 1
#fi

