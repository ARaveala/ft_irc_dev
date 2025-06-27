import socket
import threading
import time
import sys
import traceback
import uuid # For generating unique PING tokens

# --- Configuration ---
HOST = '127.0.0.1'
PORT = 6667
PASSWORD = 'testpass' 
CHANNEL = '#testchannel'
CHANNEL_KEY = '#keychannel'
KEY = 'secretkey'
CHANNEL_LIMIT = '#limit'

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
        self.message_lock = threading.Lock() # Lock to protect received_messages access

    def _send_command(self, command):
        """Sends a command to the IRC server, appending CRLF."""
        try:
            full_command = f"{command}\r\n"
            print(f"[Client {self.client_id} ({self.nickname})] Sending: {command.strip()}") # .strip() for cleaner log
            if self.sock:
                self.sock.sendall(full_command.encode('utf-8'))
            else:
                print(f"[Client {self.client_id} ({self.nickname})] Warning: Attempted to send command '{command.strip()}' on a closed or uninitialized socket.", file=sys.stderr)
        except socket.error as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] SOCKET ERROR during send: {e} at line {line_num}", file=sys.stderr)
            self.stop_listening.set()
        except Exception as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            line_num = exc_tb.tb_lineno
            print(f"[Client {self.client_id} ({self.nickname})] ERROR during send: {e} at line {line_num}", file=sys.stderr)
            self.stop_listening.set()

    def _listen_for_responses(self):
        """Continuously listens for and prints server responses, handling PINGs."""
        buffer = ""
        try:
            while not self.stop_listening.is_set():
                if not self.sock: 
                    print(f"[Client {self.client_id} ({self.nickname})] Listener stopping: Socket is closed or invalid.", file=sys.stderr)
                    break

                self.sock.settimeout(0.5) 
                try:
                    data = self.sock.recv(4096).decode('utf-8', errors='ignore')
                except socket.timeout:
                    continue
                
                if not data:
                    print(f"[Client {self.client_id} ({self.nickname})] Server closed connection.")
                    break
                
                buffer += data
                lines = buffer.split('\r\n')
                buffer = lines.pop() 

                for line in lines:
                    if line:
                        print(f"[Client {self.client_id} ({self.nickname})] Received: {line}")
                        with self.message_lock:
                            self.received_messages.append(line)

                        if line.startswith("PING :"):
                            ping_token = line.split(":", 1)[1]
                            pong_command = f"PONG :{ping_token}"
                            self._send_command(pong_command)
                            print(f"[Client {self.client_id} ({self.nickname})] Responded to server PING with PONG.")
                        
                        parts = line.split(' ')
                        if len(parts) >= 3:
                            numeric_code = parts[1]
                            target_nick = parts[2]
                            if numeric_code in ["001", "002", "003", "004"] and target_nick == self.nickname:
                                print(f"[Client {self.client_id} ({self.nickname})] Detected welcome message {numeric_code}.")
                                self.registered.set()
                            elif numeric_code == "376" and target_nick == self.nickname:
                                print(f"[Client {self.client_id} ({self.nickname})] Detected End of MOTD (376).")
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
            self.stop_listening.set()

    def connect_and_register(self):
        """Connects to the server and performs the registration sequence."""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.host, self.port))
            print(f"[Client {self.client_id} ({self.nickname})] Connected to {self.host}:{self.port}")

            self.listener_thread = threading.Thread(target=self._listen_for_responses)
            self.listener_thread.start()

            self._send_command("CAP LS 302")
            self._send_command(f"PASS {self.password}")
            self._send_command(f"NICK {self.nickname}")
            self._send_command(f"USER {self.username} 0 * :{self.realname}")

            print(f"[Client {self.client_id} ({self.nickname})] Waiting for server welcome messages (001-004 / 376)...")
            if not self.registered.wait(timeout=10):
                print(f"[Client {self.client_id} ({self.nickname})] Full registration timed out or failed.", file=sys.stderr)
                self.stop_listening.set()
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
        self.stop_listening.set()
        time.sleep(0.1) 
        
        if self.listener_thread and self.listener_thread.is_alive():
            print(f"[Client {self.client_id} ({self.nickname})] Joining listener thread...")
            self.listener_thread.join(timeout=1)
            if self.listener_thread.is_alive():
                print(f"[Client {self.client_id} ({self.nickname})] Listener thread did not terminate cleanly.", file=sys.stderr)
        
        current_sock = self.sock 

        if current_sock:
            try:
                current_sock.shutdown(socket.SHUT_RDWR)
                current_sock.close()
                print(f"[Client {self.client_id} ({self.nickname})] Connection closed.")
            except OSError as e:
                print(f"[Client {self.client_id} ({self.nickname})] Error during socket shutdown/close (possibly already closed or invalid): {e}", file=sys.stderr)
            finally:
                self.sock = None
        else:
            print(f"[Client {self.client_id} ({self.nickname})] No active socket to close or already closed.")

    def wait_for_response(self, timeout=5, numeric_code=None, message_contains=None):
        """
        Waits for a specific IRC numeric response or a message containing a substring.
        Returns True if found, False otherwise.
        """
        start_time = time.time()
        initial_messages_len = 0
        with self.message_lock:
            initial_messages_len = len(self.received_messages)

        while time.time() - start_time < timeout:
            with self.message_lock:
                # Check only new messages since the last check
                for i in range(initial_messages_len, len(self.received_messages)):
                    line = self.received_messages[i]
                    
                    found_numeric = False
                    if numeric_code:
                        parts = line.split(' ')
                        if len(parts) >= 2 and parts[1] == str(numeric_code):
                            found_numeric = True
                    
                    found_message = False
                    if message_contains:
                        if message_contains in line:
                            found_message = True
                    
                    if (numeric_code and found_numeric) or \
                       (message_contains and found_message) or \
                       (numeric_code is None and message_contains is None and line): # Any message
                        print(f"[Client {self.client_id} ({self.nickname})] Expected response received: '{line}'")
                        return True
                initial_messages_len = len(self.received_messages) # Update index for next iteration
            time.sleep(0.1) # Prevent busy-waiting
        print(f"[Client {self.client_id} ({self.nickname})] Timed out waiting for response (numeric={numeric_code}, message='{message_contains}')", file=sys.stderr)
        return False

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
        time.sleep(0.1) 

    registration_success = True
    for i, thread in enumerate(threads):
        print(f"[Main] Waiting for client {clients[i].nickname} registration thread to finish...")
        thread.join(timeout=15)
        if thread.is_alive():
            print(f"[Main] Client thread for {clients[i].nickname} is still alive after join timeout. Registration likely failed.", file=sys.stderr)
            registration_success = False

    if not all(c.registered.is_set() for c in clients) or not registration_success:
        print("\n--- Test Failed: Not all clients registered successfully. Exiting. ---", file=sys.stderr)
        for client in clients:
            client.close()
        return

    time.sleep(1) 
    print("\n--- All clients registered. Proceeding to channel operations. ---")

    client1 = clients[0]
    client2 = clients[1]
    client3 = clients[2] # All clients are used in tests

    # 2. Clients join the same channel
    print(f"\n--- Phase 2: All clients joining channel {CHANNEL} ---")
    client1._send_command(f"JOIN {CHANNEL}")
    client1.wait_for_response(numeric_code=332, message_contains=CHANNEL) # RPL_TOPIC or RPL_NOTOPIC
    client2._send_command(f"JOIN {CHANNEL}")
    client2.wait_for_response(numeric_code=332, message_contains=CHANNEL)
    client3._send_command(f"JOIN {CHANNEL}")
    client3.wait_for_response(numeric_code=332, message_contains=CHANNEL)
    time.sleep(1.5)

    # 3. Client1 (operator) sets MODE +o on themselves
    print(f"\n--- Phase 3: {client1.nickname} attempting to set MODE +o on self in {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL} +o {client1.nickname}")
    assert client1.wait_for_response(message_contains=f"MODE {CHANNEL} +o {client1.nickname}") # Check for server echo
    time.sleep(1)

    # 4. Operator (Client1) sets channel mode +i (invite-only)
    print(f"\n--- Phase 4: {client1.nickname} setting channel mode +i on {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL} +i")
    assert client1.wait_for_response(message_contains=f"MODE {CHANNEL} +i")
    time.sleep(1)

    # 5. Operator (Client1) kicks Client2 from the channel
    print(f"\n--- Phase 5: {client1.nickname} kicking {client2.nickname} from {CHANNEL} ---")
    client1._send_command(f"KICK {CHANNEL} {client2.nickname} :Kicked for test")
    assert client1.wait_for_response(message_contains=f"KICK {CHANNEL} {client2.nickname}")
    assert client2.wait_for_response(message_contains=f"KICK {CHANNEL} {client2.nickname}") # Client2 should also see the KICK
    time.sleep(1.5)

    # 6. Client2 tries to join without invite (should fail)
    print(f"\n--- Phase 6: {client2.nickname} attempting to join {CHANNEL} (should fail due to +i) ---")
    client2._send_command(f"JOIN {CHANNEL}")
    assert client2.wait_for_response(numeric_code=473, message_contains="Cannot join channel (+i)") # ERR_INVITEONLYCHAN
    time.sleep(1)

    # 7. Operator (Client1) sends INVITE to Client2
    print(f"\n--- Phase 7: {client1.nickname} inviting {client2.nickname} to {CHANNEL} ---")
    client1._send_command(f"INVITE {client2.nickname} {CHANNEL}")
    assert client1.wait_for_response(numeric_code=341, message_contains=client2.nickname) # RPL_INVITING
    assert client2.wait_for_response(message_contains=f"INVITE {client2.nickname} {CHANNEL}") # Client2 receives INVITE
    time.sleep(1)

    # 8. Invited Client2 tries to join (should succeed)
    print(f"\n--- Phase 8: {client2.nickname} attempting to join {CHANNEL} (should succeed after invite) ---")
    client2._send_command(f"JOIN {CHANNEL}")
    assert client2.wait_for_response(numeric_code=332, message_contains=CHANNEL)
    time.sleep(2)

    # --- New Error Reply Tests ---
    print("\n--- Phase 9: Testing Error Replies ---")

    # Test: Unknown mode flag (+z) -> 472 (ERR_UNKNOWNMODE)
    print(f"\n--- Testing: Unknown mode flag (+z) on {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL} +z")
    assert client1.wait_for_response(numeric_code=472, message_contains="is unknown mode char to me")
    time.sleep(0.5)

    # Test: User not on channel (KICK non-existent) -> 401/441 (ERR_NOSUCHNICK/ERR_USERNOTINCHANNEL)
    print(f"\n--- Testing: KICK non-existent user 'NonExistentNick' from {CHANNEL} ---")
    client1._send_command(f"KICK {CHANNEL} NonExistentNick :Go away")
    # Server might send 401 if it checks for the nick being registered at all, or 441 if it checks channel membership.
    # We'll assert on either, prioritizing 441 as per prompt.
    assert client1.wait_for_response(numeric_code=441) or client1.wait_for_response(numeric_code=401)
    time.sleep(0.5)

    # Test: User not on channel (INVITE Client3 after it leaves) -> 401/443
    print(f"\n--- Testing: INVITE {client3.nickname} to {CHANNEL} after they leave ---")
    # Ensure client3 is NOT on the channel for this test.
    client3._send_command(f"PART {CHANNEL}")
    assert client3.wait_for_response(message_contains=f"PART {CHANNEL}")
    time.sleep(0.5)
    
    client1._send_command(f"INVITE {client3.nickname} {CHANNEL}")
    # 443 (ERR_USERONCHANNEL) if the user is in the channel (but our test aims for not in channel)
    # 401 (ERR_NOSUCHNICK) if the user isn't on any channel or doesn't exist
    # If the user is registered but not on *that* channel, a 341 RPL_INVITING is sent to the inviter
    # and a PRIVMSG is sent to the invited. The error only comes if the nickname doesn't exist.
    # The prompt specifically asks for 441 or 443. Given Client3 is registered but not in the channel,
    # the server should typically send 341 to inviter and INVITE to invitee. If an error is returned,
    # it might imply the user is unknown or already on the channel.
    # For now, let's check for 341 if successful, as that's the standard.
    # If the server incorrectly sends an error here, the test will fail and you can check your server's logic.
    assert client1.wait_for_response(numeric_code=341, message_contains=client3.nickname) # RPL_INVITING
    assert client3.wait_for_response(message_contains=f"INVITE {client3.nickname} {CHANNEL}") # Client3 receives the INVITE
    time.sleep(0.5)


    # Test: Operator-only command by non-operator (TOPIC) -> 482 (ERR_CHANOPRIVSNEEDED)
    print(f"\n--- Testing: {client2.nickname} (non-op) setting TOPIC on {CHANNEL} ---")
    client2._send_command(f"TOPIC {CHANNEL} :New topic from non-op")
    assert client2.wait_for_response(numeric_code=482, message_contains="You're not channel operator") # ERR_CHANOPRIVSNEEDED
    time.sleep(0.5)

    # Test: Operator-only command by non-operator (MODE +i) -> 482 (ERR_CHANOPRIVSNEEDED)
    print(f"\n--- Testing: {client2.nickname} (non-op) setting MODE +i on {CHANNEL} ---")
    client2._send_command(f"MODE {CHANNEL} +i")
    assert client2.wait_for_response(numeric_code=482, message_contains="You're not channel operator")
    time.sleep(0.5)

    # Test: Missing parameters (MODE #channel) -> 461 (ERR_NEEDMOREPARAMS)
    print(f"\n--- Testing: Missing parameters for MODE {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL}")
    assert client1.wait_for_response(numeric_code=461, message_contains="Not enough parameters") # ERR_NEEDMOREPARAMS
    time.sleep(0.5)

    # Test: Bad parameters (MODE +o NonExistentNick) -> 401 (ERR_NOSUCHNICK)
    print(f"\n--- Testing: Bad parameters for MODE +o 'NonExistentNick' on {CHANNEL} ---")
    client1._send_command(f"MODE {CHANNEL} +o NonExistentNick")
    assert client1.wait_for_response(numeric_code=401, message_contains="No such nick/channel") # ERR_NOSUCHNICK
    time.sleep(0.5)
    
    # --- Channel Mode Specific Tests ---
    print("\n--- Phase 10: Testing Channel Modes (+k, +l) ---")

    # Clean up old channel occupants for a fresh test
    client1._send_command(f"PART {CHANNEL}")
    client2._send_command(f"PART {CHANNEL}")
    client3._send_command(f"PART {CHANNEL}")
    assert client1.wait_for_response(message_contains=f"PART {CHANNEL}")
    assert client2.wait_for_response(message_contains=f"PART {CHANNEL}")
    assert client3.wait_for_response(message_contains=f"PART {CHANNEL}")
    time.sleep(1)

    # Test: Channel with key (+k)
    print(f"\n--- Testing: Channel with key {CHANNEL_KEY} (+k) ---")
    client1._send_command(f"JOIN {CHANNEL_KEY}")
    client1.wait_for_response(numeric_code=332, message_contains=CHANNEL_KEY)
    time.sleep(0.5)
    client1._send_command(f"MODE {CHANNEL_KEY} +k {KEY}")
    assert client1.wait_for_response(message_contains=f"MODE {CHANNEL_KEY} +k {KEY}")
    time.sleep(0.5)

    print(f"--- {client2.nickname} trying to JOIN {CHANNEL_KEY} without key (should fail with 475) ---")
    client2._send_command(f"JOIN {CHANNEL_KEY}")
    assert client2.wait_for_response(numeric_code=475, message_contains="Cannot join channel (+k)") # ERR_BADCHANNELKEY
    time.sleep(0.5)

    print(f"--- {client2.nickname} trying to JOIN {CHANNEL_KEY} with correct key (should succeed) ---")
    client2._send_command(f"JOIN {CHANNEL_KEY} {KEY}")
    assert client2.wait_for_response(numeric_code=332, message_contains=CHANNEL_KEY) # RPL_TOPIC
    time.sleep(1)

    # Test: Channel Limit (+l)
    print(f"\n--- Testing: Channel limit {CHANNEL_LIMIT} (+l) ---")
    client1._send_command(f"PART {CHANNEL_KEY}") # Client1 leaves keyed channel to join limit
    assert client1.wait_for_response(message_contains=f"PART {CHANNEL_KEY}")
    time.sleep(0.5)

    client2._send_command(f"PART {CHANNEL_KEY}") # Client2 leaves keyed channel
    assert client2.wait_for_response(message_contains=f"PART {CHANNEL_KEY}")
    time.sleep(0.5)

    client1._send_command(f"JOIN {CHANNEL_LIMIT}")
    client1.wait_for_response(numeric_code=332, message_contains=CHANNEL_LIMIT)
    time.sleep(0.5)
    client1._send_command(f"MODE {CHANNEL_LIMIT} +l 1")
    assert client1.wait_for_response(message_contains=f"MODE {CHANNEL_LIMIT} +l 1")
    time.sleep(0.5)

    print(f"--- {client2.nickname} trying to JOIN {CHANNEL_LIMIT} (should fail with 471 due to limit) ---")
    client2._send_command(f"JOIN {CHANNEL_LIMIT}")
    assert client2.wait_for_response(numeric_code=471, message_contains="Cannot join channel (+l)") # ERR_CHANNELISFULL
    time.sleep(1)
    
    # --- Stress Section ---
    print("\n--- Phase 11: Stress Testing ---")

    # Stress Test 1: One client rapidly sends malformed or repeated commands
    print("\n--- Stress Test 1: Rapidly sending malformed/repeated commands with Client3 ---")
    num_stress_commands = 20
    for i in range(num_stress_commands):
        # Malformed commands
        client3._send_command(f"GARBAGE {i}")
        client3._send_command(f"JOIN #malformed_channel_ {i % 2}") # Incomplete channel name
        client3._send_command(f"MODE {CHANNEL} +xyz") # Absurd mode flags
        client3._send_command(f"NICK new_nick_too_long_for_irc_standards_probably_{i}") # Too long nick
        # Repeated commands
        client3._send_command(f"PRIVMSG {CHANNEL} :Stress message {i}")
        time.sleep(0.05) # Send rapidly
    print(f"--- {client3.nickname} finished sending {num_stress_commands*5} stress commands. ---")
    time.sleep(2) # Give server time to process or reject these

    # Stress Test 2: Another client disconnects mid-command
    print("\n--- Stress Test 2: Client abruptly disconnects mid-command ---")
    stress_client_disconnect = IRCClient(
        nickname="MidDisc",
        username="miduser",
        realname="Mid Disconnect",
        password=PASSWORD,
        host=HOST,
        port=PORT,
        client_id=4 # New client ID for this test
    )
    
    disconnect_thread = threading.Thread(target=stress_client_disconnect.connect_and_register)
    disconnect_thread.start()
    
    # Wait just long enough for it to connect and send NICK, but not USER fully
    time.sleep(0.5) 
    
    # Abruptly close the socket. This will likely cause errors on the client side,
    # but we are testing server robustness here.
    if stress_client_disconnect.sock:
        try:
            print(f"[Client {stress_client_disconnect.client_id} ({stress_client_disconnect.nickname})] Abruptly closing socket mid-command...")
            stress_client_disconnect.sock.close()
            stress_client_disconnect.sock = None # Manually set to None
            stress_client_disconnect.stop_listening.set() # Ensure listener stops
        except Exception as e:
            print(f"[Client {stress_client_disconnect.client_id} ({stress_client_disconnect.nickname})] Error during abrupt close: {e}", file=sys.stderr)
    
    disconnect_thread.join(timeout=5) # Wait for its thread to attempt to terminate
    if disconnect_thread.is_alive():
        print(f"[Main] Stress client 'MidDisc' thread is still alive after abrupt disconnect. Server might be holding resources.", file=sys.stderr)
    else:
        print(f"[Main] Stress client 'MidDisc' thread terminated cleanly after abrupt disconnect. Good.", file=sys.stderr)
    time.sleep(2) # Give server time to clean up on its side

    # --- Test complete ---
    print("\n--- Test sequence finished. Please review the output above for successes and failures. ---")

    # Clean up: Close all clients (including any remaining stress clients)
    print("\n--- Cleaning up connections ---")
    for client in clients:
        client.close()
    
    # Close the abrupt disconnect client if it's still alive (it probably closed its own socket)
    stress_client_disconnect.close()

    print("\n--- Script finished ---")

if __name__ == "__main__":
    run_irc_test()
