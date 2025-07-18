import socket
import threading
import time
import sys
import traceback
import uuid # For generating unique PING tokens

# --- Configuration ---
HOST = '127.0.0.1'
PORT = 6667
PASSWORD = 'testpass' # Updated password as per new prompt
CHANNEL = '#testchannel'

# --- IRC Client Class ---
class IRCClient:
    def __init__(self, nickname, username, realname, password, host, port, client_id):
        self.nickname = nickname
        self.username = username
        self.realname = realname
        self.password = password
        self.host = host
        self.port = port
        self.client_id = client_id
        self.sock = None
        self.listener_thread = None # Store reference to the listener thread
        self.registered = threading.Event() # Event to signal successful full registration (001-004)
        self.stop_listening = threading.Event() # Event to stop the listener thread
        self.received_messages = [] # To store messages received by the listener thread

    def _send_command(self, command):
        """Sends a command to the IRC server, appending CRLF."""
        try:
            full_command = f"{command}\r\n"
            print(f"[Client {self.client_id} ({self.nickname})] Sending: {command.strip()}") # .strip() for cleaner log
            # Ensure socket is not None before sending
            if self.sock:
                self.sock.sendall(full_command.encode('utf-8'))
            else:
                print(f"[Client {self.client_id} ({self.nickname})] Warning: Attempted to send command '{command.strip()}' on a closed or uninitialized socket.", file=sys.stderr)
        except socket.error as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] SOCKET ERROR during send: {e} at line {line_num}", file=sys.stderr)
            self.stop_listening.set() # Stop listening thread on send error
        except Exception as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] ERROR during send: {e} at line {line_num}", file=sys.stderr)
            self.stop_listening.set() # Stop listening thread on send error

    def _listen_for_responses(self):
        """Continuously listens for and prints server responses, handling PINGs."""
        buffer = ""
        try:
            while not self.stop_listening.is_set():
                # Check if socket is still valid before trying to recv
                if not self.sock: # Self.sock can become None if main thread closes it first
                    print(f"[Client {self.client_id} ({self.nickname})] Listener stopping: Socket is closed or invalid.", file=sys.stderr)
                    break

                # Set a small timeout for recv to allow graceful exit if stop_listening is set
                # without blocking indefinitely.
                self.sock.settimeout(0.5) 
                try:
                    data = self.sock.recv(4096).decode('utf-8', errors='ignore')
                except socket.timeout:
                    # No data received within the timeout, check stop_listening flag again
                    continue
                
                if not data:
                    print(f"[Client {self.client_id} ({self.nickname})] Server closed connection.")
                    break
                
                buffer += data
                lines = buffer.split('\r\n')
                buffer = lines.pop() # Keep incomplete last line in buffer

                for line in lines:
                    if line: # Ensure line is not empty
                        print(f"[Client {self.client_id} ({self.nickname})] Received: {line}")
                        self.received_messages.append(line) # Store for potential later checking

                        # Handle server-initiated PINGs (respond with PONG)
                        if line.startswith("PING :"):
                            # Extract the token after "PING :"
                            ping_token = line.split(":", 1)[1]
                            pong_command = f"PONG :{ping_token}"
                            self._send_command(pong_command)
                            print(f"[Client {self.client_id} ({self.nickname})] Responded to server PING with PONG.")
                        
                        # Check for welcome messages (001-004) to confirm full registration
                        # Pattern example: ':server.name 001 ClientNick :Welcome message'
                        parts = line.split(' ')
                        if len(parts) >= 3:
                            numeric_code = parts[1]
                            target_nick = parts[2]
                            # Check if the numeric code is one of the welcome messages and matches our nickname
                            # Server may use the initial nickname in 001 even if it's auto-changed later.
                            if numeric_code in ["001", "002", "003", "004"] and target_nick == self.nickname:
                                print(f"[Client {self.client_id} ({self.nickname})] Detected welcome message {numeric_code}.")
                                self.registered.set() # Signal that full registration is complete
                            # Also consider MOTD end as potential signal if 001-004 are inconsistent
                            # The prompt mentions "possibly 375–376 (MOTD)"
                            elif numeric_code == "376" and target_nick == self.nickname:
                                print(f"[Client {self.client_id} ({self.nickname})] Detected End of MOTD (376).")
                                # If 001-004 were not received, this might be the last indicator
                                if not self.registered.is_set():
                                    self.registered.set()


        except socket.error as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] SOCKET ERROR during receive: {e} at line {line_num}", file=sys.stderr)
        except Exception as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] ERROR during receive: {e} at line {line_num}", file=sys.stderr)
        finally:
            self.stop_listening.set() # Ensure the loop terminates
            # IMPORTANT: Do NOT close socket or set self.sock = None here.
            # This is handled by the main thread's close method to prevent race conditions.


    def connect_and_register(self):
        """Connects to the server and performs the registration sequence."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Set socket to blocking initially for connect, then switch to non-blocking or rely on timeout.
            # For this simple script, default blocking is fine; the listener thread handles recv.
            self.sock.connect((self.host, self.port))
            print(f"[Client {self.client_id} ({self.nickname})] Connected to {self.host}:{self.port}")

            # Start listener thread immediately after connecting
            # Not a daemon thread anymore, so it must be explicitly joined.
            self.listener_thread = threading.Thread(target=self._listen_for_responses)
            self.listener_thread.start()

            # --- Registration commands ---
            self._send_command("CAP LS 302")
            self._send_command(f"PASS {self.password}")
            self._send_command(f"NICK {self.nickname}")
            self._send_command(f"USER {self.username} 0 * :{self.realname}")

            # Wait for full registration to complete (welcome messages 001-004 and potentially 376)
            print(f"[Client {self.client_id} ({self.nickname})] Waiting for server welcome messages (001-004 / 376)...")
            if not self.registered.wait(timeout=10): # Wait up to 10 seconds for full registration
                print(f"[Client {self.client_id} ({self.nickname})] Full registration timed out or failed.", file=sys.stderr)
                self.stop_listening.set() # Signal listener to stop
                return False
            print(f"[Client {self.client_id} ({self.nickname})] Full registration successful!")
            return True

        except ConnectionRefusedError:
            print(f"[Client {self.client_id} ({self.nickname})] Connection refused. Is the server running at {self.host}:{self.port}?", file=sys.stderr)
            return False
        except socket.error as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] SOCKET ERROR during connection/registration: {e} at line {line_num}", file=sys.stderr)
            return False
        except Exception as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] ERROR during connection/registration: {e} at line {line_num}", file=sys.stderr)
            return False

    def close(self):
        """Closes the client connection and joins its listener thread."""
        self.stop_listening.set() # Signal listener thread to stop
        
        # Give the listener a moment to cleanly exit its loop
        time.sleep(0.1) 
        
        # Attempt to join the listener thread
        if self.listener_thread and self.listener_thread.is_alive():
            print(f"[Client {self.client_id} ({self.nickname})] Joining listener thread...")
            self.listener_thread.join(timeout=1) # Give it a small timeout to join
            if self.listener_thread.is_alive():
                print(f"[Client {self.client_id} ({self.nickname})] Listener thread did not terminate cleanly.", file=sys.stderr)
        
        # Safely get the socket reference for closing
        current_sock = self.sock 

        if current_sock: # Only attempt to close if a socket object exists
            try:
                # Attempt graceful shutdown first
                current_sock.shutdown(socket.SHUT_RDWR)
                current_sock.close()
                print(f"[Client {self.client_id} ({self.nickname})] Connection closed.")
            except OSError as e:
                # Handle cases where shutdown/close fails (e.g., socket already closed by peer)
                print(f"[Client {self.client_id} ({self.nickname})] Error during socket shutdown/close (possibly already closed or invalid): {e}", file=sys.stderr)
            finally:
                # Always ensure self.sock is set to None after attempting to close it
                self.sock = None
        else:
            print(f"[Client {self.client_id} ({self.nickname})] No active socket to close or already closed.")


# --- Main Test Script Logic ---
def run_irc_test():
    clients = []
    threads = []
    
    # Client configurations
    client_configs = [
        {"nick": "Client1Nick", "user": "user1", "real": "Real Name One"},
        {"nick": "Client2Nick", "user": "user2", "real": "Real Name Two"},
        {"nick": "Client3Nick", "user": "user3", "real": "Real Name Three"}
    ]

    # 1. Connect and Register all clients concurrently
    print("\n--- Phase 1: Connecting and Registering Clients ---")
    for i, config in enumerate(client_configs):
        client = IRCClient(
            nickname=config["nick"],
            username=config["user"],
            realname=config["real"],
            password=PASSWORD,
            host=HOST,
            port=PORT,
            client_id=i+1
        )
        clients.append(client)
        
        thread = threading.Thread(target=client.connect_and_register)
        thread.start()
        threads.append(thread)
        # Give a small moment for each client's thread to start connecting
        time.sleep(0.1) 

    # Wait for all client registration threads to finish
    registration_success = True
    for i, thread in enumerate(threads):
        # Increased join timeout for the entire connection/registration process
        print(f"[Main] Waiting for client {clients[i].nickname} registration thread to finish...")
        thread.join(timeout=15) # Increased join timeout for registration
        if thread.is_alive():
            print(f"[Main] Client thread for {clients[i].nickname} is still alive after join timeout. Registration likely failed.", file=sys.stderr)
            registration_success = False

    # Check if all clients successfully registered before proceeding
    if not all(c.registered.is_set() for c in clients) or not registration_success:
        print("\n--- Test Failed: Not all clients registered successfully. Exiting. ---", file=sys.stderr)
        for client in clients:
            client.close()
        return

    # Give a brief moment for server to settle after all registrations
    time.sleep(1) 
    print("\n--- All clients registered. Proceeding to channel operations. ---")

    # Get client references for easier access
    client1 = clients[0]
    client2 = clients[1]
    client3 = clients[2] if len(clients) > 2 else None

    # 2. Clients join the same channel
    print(f"\n--- Phase 2: All clients joining channel {CHANNEL} ---")
    client1._send_command(f"JOIN {CHANNEL}")
    time.sleep(0.5) # Give server time to process
    client2._send_command(f"JOIN {CHANNEL}")
    time.sleep(0.5) # Give server time to process
    if client3:
        client3._send_command(f"JOIN {CHANNEL}")
        time.sleep(0.5)

    # Give a moment for JOIN messages and RPL_NAMREPLY to be processed
    time.sleep(1.5)

    # 3. Client1 (operator) sets MODE +o on themselves (assuming first joinee gets +o, or server allows)
    # Note: In a real IRC scenario, usually the first person joining an empty channel gets +o.
    # If your server doesn't do this automatically, this command might fail if Client1 isn't already op.
    print(f"\n--- Phase 3: {client1.nickname} attempting to set MODE +o on self in {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL} +o {client1.nickname}")
    time.sleep(1)

    # 4. Operator (Client1) sets channel mode +i (invite-only)
    print(f"\n--- Phase 4: {client1.nickname} setting channel mode +i on {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL} +i")
    time.sleep(1)

    # 5. Operator (Client1) kicks Client2 from the channel
    print(f"\n--- Phase 5: {client1.nickname} kicking {client2.nickname} from {CHANNEL} ---")
    client1._send_command(f"KICK {CHANNEL} {client2.nickname} :Kicked for testing +i mode")
    time.sleep(1.5) # Give server time to process kick and send PART/KICK messages

    # 6. Client2 tries to join without invite (should fail)
    print(f"\n--- Phase 6: {client2.nickname} attempting to join {CHANNEL} (should fail due to +i) ---")
    client2._send_command(f"JOIN {CHANNEL}")
    time.sleep(2) # Give more time for the server to send ERR_INVITEONLYCHAN (473)

    # 7. Operator (Client1) sends INVITE to Client2
    print(f"\n--- Phase 7: {client1.nickname} inviting {client2.nickname} to {CHANNEL} ---")
    client1._send_command(f"INVITE {client2.nickname} {CHANNEL}")
    time.sleep(1) # Give server time to process invite and send RPL_INVITING (341)

    # 8. Invited Client2 tries to join (should succeed)
    print(f"\n--- Phase 8: {client2.nickname} attempting to join {CHANNEL} (should succeed after invite) ---")
    client2._send_command(f"JOIN {CHANNEL}")
    time.sleep(2) # Give time for successful join messages

    # --- Test complete ---
    print("\n--- Test sequence finished. Please review the output above. ---")

    # Clean up: Close all client connections
    print("\n--- Cleaning up connections ---")
    for client in clients:
        client.close()
        
    print("\n--- Script finished ---")

if __name__ == "__main__":
    run_irc_test()
