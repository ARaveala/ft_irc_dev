# tests/test_basic_connection.py
import socket
import subprocess
import time
import unittest

# --- Configuration ---
SERVER_HOST = '127.0.0.1'
SERVER_PORT = 6667
SERVER_PASSWORD = 'pass'
SERVER_PATH = '../v1.2/ft_irc' # Path to your compiled executable, relative this directory

# --- Helper Functions (copy from previous example) ---
def send_irc_command(sock, command_str):
    sock.sendall(command_str.encode('utf-8') + b'\r\n')

def recv_irc_response(sock, timeout=1.0):
    sock.settimeout(timeout)
    buffer = b''
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            buffer += chunk
            if b'\r\n' in buffer:
                break # Read until you get a full message
    except socket.timeout:
        pass
    return buffer.decode('utf-8').strip()

# --- Test Class ---
class TestBasicConnection(unittest.TestCase):
    server_process = None

    @classmethod
    def setUpClass(cls):
        # Start the IRC server once for all tests in this class
        print(f"\nStarting server: {SERVER_PATH} {SERVER_PORT} {SERVER_PASSWORD}")
        cls.server_process = subprocess.Popen(
            [SERVER_PATH, str(SERVER_PORT), SERVER_PASSWORD],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        time.sleep(0.5) # Give server time to start

    @classmethod
    def tearDownClass(cls):
        # Stop the IRC server after all tests in this class
        print(f"Stopping server (PID: {cls.server_process.pid})")
        cls.server_process.terminate()
        cls.server_process.wait(timeout=1)
        if cls.server_process.poll() is None:
            cls.server_process.kill()
        stdout, stderr = cls.server_process.communicate()
        print(f"Server stdout:\n{stdout.decode()}")
        print(f"Server stderr:\n{stderr.decode()}")

    def setUp(self):
        # Connect a new client for each test method
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((SERVER_HOST, SERVER_PORT))
        # Handle initial PING from server if it sends one
        initial_response = recv_irc_response(self.sock)
        if initial_response.startswith('PING'):
            send_irc_command(self.sock, initial_response.replace('PING', 'PONG'))

    def tearDown(self):
        # Close client connection after each test method
        self.sock.close()

    def test_client_connects(self):
        # Just test that a connection can be established without error
        # The setUp/tearDown handle the actual connect/disconnect.
        # You could add an assertion here if your server sends an immediate message upon connection.
        # For now, simply reaching this point without exception is a success for basic connect.
        print("Test: Client successfully connected and disconnected.")
        pass # No specific assertions needed for just connecting/disconnecting

if __name__ == '__main__':
    unittest.main()
