#!/bin/bash


PID_DIR="log_comp/pids"
PIPE_DIR="log_comp/client_inputs"

for PID_FILE in "$PID_DIR"/*.pid; do
  # Extract nickname from filename
  NICKNAME=$(basename "$PID_FILE" .pid)
  PID=$(cat "$PID_FILE")

  # Kill netcat if alive
  if kill -0 "$PID" 2>/dev/null; then
    echo "Killing netcat client '$NICKNAME' (PID $PID)..."
    kill "$PID"
  else
    echo "Netcat for '$NICKNAME' already inactive or exited"
  fi
 # Kill tail -f queue feeder
  if [ -f "$QUEUE_PID_FILE" ]; then
    TAIL_PID=$(cat "$QUEUE_PID_FILE")
    if kill -0 "$TAIL_PID" 2>/dev/null; then
      echo "Stopping queue feeder '$NICKNAME' (PID $TAIL_PID)..."
      kill "$TAIL_PID"
    fi
  fi
  # Kill queue feeder if present
  QUEUE_PID_FILE="${PID_DIR}/${NICKNAME}_queue.pid"
  if [ -f "$QUEUE_PID_FILE" ]; then
    QUEUE_PID=$(cat "$QUEUE_PID_FILE")
    if kill -0 "$QUEUE_PID" 2>/dev/null; then
      echo "Stopping command queue for '$NICKNAME' (PID $QUEUE_PID)..."
      kill "$QUEUE_PID"
    else
      echo "Queue process for '$NICKNAME' not active"
    fi
  fi
  # Clean up pipe if it's still on disk
  PIPE="${PIPE_DIR}/${NICKNAME}.in"
  if [ -p "$PIPE" ]; then
    echo "Removing pipe for '$NICKNAME'"
    rm -f "$PIPE"
  elif [ -e "$PIPE" ]; then
    echo "Pipe path exists but is not a pipe — removing anyway"
    rm -f "$PIPE"
  fi
  PIPE="log_comp/client_inputs/${NICKNAME}.in"
  if [ -e "$PIPE" ] || [ -p "$PIPE" ]; then
    echo "Checking for lingering processes on $PIPE..."
    fuser -k "$PIPE" 2>/dev/null
    sleep 0.3
    rm -f "$PIPE"
  fi
done

# Optional: wait for inode release
sleep 0.5
echo "✅ All clients and pipes cleaned up."