# tests/irc_tester_utils.py
import socket
import subprocess
import time
import unittest
import io # Ensure this is imported for BytesIO usage in run_all_tests.py

# --- Configuration ---
SERVER_HOST = '127.0.0.1' # Keep this as is
SERVER_PORT = 6667
SERVER_PASSWORD = 'pass'
SERVER_PATH = '../v1.2/localhost'

# --- IRC Reply Codes (Comprehensive list for testing) ---
# For successful operations
RPL_WELCOME = "001"
RPL_YOURHOST = "002"
RPL_CREATED = "003"
RPL_MYINFO = "004"
RPL_BOUNCE = "005" # Often used for server links, not mandatory for project

RPL_UMODEIS = "221"
RPL_LUSERCLIENT = "251"
RPL_LUSEROP = "252"
RPL_LUSERUNKNOWN = "253"
RPL_LUSERCHANNELS = "254"
RPL_LUSERME = "255"
RPL_ADMINME = "256"
RPL_ADMINLOC1 = "257"
RPL_ADMINLOC2 = "258"
RPL_ADMINEMAIL = "259"
RPL_TRACELINK = "261"
RPL_ENDOFSTATS = "219"

RPL_AWAY = "301"
RPL_UNAWAY = "305"
RPL_NOWAWAY = "306"

RPL_WHOISUSER = "311"
RPL_WHOISSERVER = "312"
RPL_WHOISOPERATOR = "313"
RPL_WHOISIDLE = "317"
RPL_ENDOFWHOIS = "318"
RPL_WHOISCHANNELS = "319"
RPL_LIST = "322"
RPL_LISTEND = "323"
RPL_CHANNELMODEIS = "324"
RPL_CREATIONTIME = "329" # Often seen with channel info
RPL_NOTOPIC = "331"
RPL_TOPIC = "332"
RPL_TOPICWHOTIME = "333"
RPL_INVITING = "341"
RPL_SUMMONING = "342"
RPL_VERSION = "351"
RPL_NAMREPLY = "353"
RPL_ENDOFNAMES = "366"
RPL_LINKS = "364"
RPL_ENDOFLINKS = "365"
RPL_BANLIST = "367"
RPL_ENDOFBANLIST = "368"
RPL_INFO = "371"
RPL_ENDOFINFO = "374"
RPL_MOTDSTART = "375"
RPL_MOTD = "372"
RPL_ENDOFMOTD = "376"
RPL_YOUREOPER = "381"
RPL_REHASHING = "382"
RPL_TIME = "391"
RPL_USERS = "392"
RPL_ENDOFUSERS = "393"
RPL_NOUSERS = "395"

# For error conditions
ERR_NOSUCHNICK = "401"
ERR_NOSUCHSERVER = "402"
ERR_NOSUCHCHANNEL = "403"
ERR_CANNOTSENDTOCHAN = "404"
ERR_TOOMANYCHANNELS = "405"
ERR_WASNOSUCHNICK = "406"
ERR_TOOMANYTARGETS = "407"
ERR_NOORIGIN = "409"
ERR_NORECIPIENT = "411"
ERR_NOTEXTTOSEND = "412"
ERR_UNKNOWNCOMMAND = "421"
ERR_NOMOTD = "422"
ERR_NONICKNAMEGIVEN = "431"
ERR_ERRONEUSNICKNAME = "432"
ERR_NICKNAMEINUSE = "433"
ERR_NICKCOLLISION = "436"
ERR_USERNOTINCHANNEL = "441"
ERR_NOTONCHANNEL = "442"
ERR_NOLOGIN = "444"
ERR_SUMMONDISABLED = "445"
ERR_USERSDISABLED = "446"
ERR_NOTREGISTERED = "451"
ERR_NEEDMOREPARAMS = "461"
ERR_ALREADYREGISTERED = "462"
ERR_NOPERMFORHOST = "463"
ERR_PASSWDMISMATCH = "464"
ERR_YOUREBANNEDKICK = "465"
ERR_CHANNELISFULL = "471"
ERR_UNKNOWNMODE = "472"
ERR_INVITEONLYCHAN = "473"
ERR_BANNEDFROMCHAN = "474"
ERR_BADCHANNELKEY = "475"
ERR_BADCHANMASK = "476"
ERR_NOCHANMODES = "477"
ERR_CHANOPRIVSNEEDED = "482"
ERR_UMODEUNKNOWNFLAG = "501"
ERR_USERSDONTMATCH = "502"


# --- Helper Functions ---
def send_irc_command(sock, command_str):
    """Sends an IRC command string to the socket, appends CRLF."""
    if not command_str.endswith('\r\n'):
        command_str += '\r\n'
    sock.sendall(command_str.encode('utf-8'))

def recv_irc_response(sock, timeout=1.0, buffer_size=4096):
    """Receives and decodes ALL IRC responses until timeout or connection close."""
    sock.settimeout(timeout)
    buffer = b''
    start_time = time.time()
    try:
        while True:
            # Check if timeout has expired
            if time.time() - start_time > timeout:
                break
            
            # Read available data (non-blocking after initial settimeout)
            try:
                chunk = sock.recv(buffer_size)
                if not chunk:
                    # Connection closed by peer
                    break
                buffer += chunk
            except BlockingIOError:
                # No data immediately available, wait a bit or continue
                time.sleep(0.01) # Small sleep to prevent busy-waiting
                continue
            except socket.timeout:
                # Timeout occurred, no more data within the window
                break
            except Exception as e:
                print(f"Error during recv: {e}")
                return [] # Return empty list on error
                
    except socket.timeout:
        pass # No data received within timeout
    except Exception as e:
        print(f"Error during recv: {e}")
        return [] # Return empty list on error

    # Decode and split into lines. Filter out empty lines.
    lines = buffer.decode('utf-8', errors='ignore').split('\r\n')
    return [line.strip() for line in lines if line.strip()]

# --- Base Test Class ---
class IrcTestBase(unittest.TestCase):
    """
    Base class for IRC tests, handling client connection/disconnection.
    Server startup/shutdown is managed by the main test runner.
    """
    # Expose global configurations as class attributes for easy access
    SERVER_HOST = SERVER_HOST
    SERVER_PORT = SERVER_PORT
    SERVER_PASSWORD = SERVER_PASSWORD

    def setUp(self):
        """Connects a new client for each test method."""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.current_nick = None
        self.current_user = None

        # Handle initial PING from server if it sends one immediately upon connection
        # Read everything that comes until timeout
        initial_response = recv_irc_response(self.sock)
        for line in initial_response:
            if line.startswith('PING'):
                pong_response = line.replace('PING', 'PONG')
                send_irc_command(self.sock, pong_response)
                # No break here, in case other messages come after PONG response
        # Optionally, read again after sending PONG if server immediately responds
        # recv_irc_response(self.sock, timeout=0.1) # Give a tiny moment for PONG response to be processed

    def tearDown(self):
        """Closes client connection after each test method."""
        if self.current_nick:
            send_irc_command(self.sock, f"QUIT :Leaving")
            recv_irc_response(self.sock, timeout=0.1) # Give server a moment to process QUIT
        self.sock.close()

    # --- Common IRC Client Actions for convenience in tests ---
    def register_client(self, nick, user="user", realname="Real Name"):
        """Registers the client with PASS, NICK, and USER commands."""
        send_irc_command(self.sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(self.sock, f"NICK {nick}")
        send_irc_command(self.sock, f"USER {user} 0 * :{realname}")
        
        # Read ALL responses from the server until timeout
        response = recv_irc_response(self.sock, timeout=2.0) # Give more time for full registration
        
        # Find the 001 message and extract the actual nickname the server used
        actual_nick_used_by_server = nick # Default to requested nick
        for line in response:
            if line.startswith(f":localhost {RPL_WELCOME}"): # Assuming server name is "localhost"
                parts = line.split(' ')
                if len(parts) >= 3:
                    actual_nick_used_by_server = parts[2] # The 3rd part is the nickname
                break
        
        self.current_nick = actual_nick_used_by_server # Update client's tracked nick after successful change
        self.current_user = user # This remains the requested username
        return response

    # This helper is needed because setUp creates self.sock,
    # but many channel tests need multiple distinct clients.
    def _register_client_on_socket(self, sock, nick, user="user", realname="Real Name"):
        """
        Helper method to register a client on a specific socket.
        This is needed when a test requires multiple concurrent clients.
        """
        # Handle initial PING from server if it sends one immediately upon connection
        initial_response = recv_irc_response(sock)
        for line in initial_response:
            if line.startswith('PING'):
                pong_response = line.replace('PING', 'PONG')
                send_irc_command(sock, pong_response)
        
        send_irc_command(sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(sock, f"NICK {nick}")
        send_irc_command(sock, f"USER {user} 0 * :{realname}")
        
        # Read ALL responses from the server until timeout
        response = recv_irc_response(sock, timeout=2.0)
        return response


    # --- Assertion Helpers (Keep as is, they are robust) ---
    def assert_irc_reply(self, response_lines, expected_code, message_part=None):
        """
        Asserts that a specific IRC numeric reply (e.g., "001", "433") is found in the response lines.
        Optionally checks for a specific message part within that reply line.
        """
        found = False
        expected_prefix = f":localhost {expected_code}" # Assuming server name is "localhost"
        for line in response_lines:
            if line.startswith(expected_prefix):
                found = True
                if message_part:
                    self.assertIn(message_part, line, f"Expected message part '{message_part}' not found in reply: {line}")
                break
        self.assertTrue(found, f"Expected IRC reply {expected_code} not found in response:\n{response_lines}")

    def assert_no_irc_reply(self, response_lines, unexpected_code):
        """
        Asserts that a specific IRC numeric reply (e.g., "001", "433") is NOT found in the response lines.
        """
        unexpected_prefix = f":localhost {unexpected_code}"
        for line in response_lines:
            if line.startswith(unexpected_prefix):
                self.fail(f"Unexpected IRC reply {unexpected_code} found in response:\n{response_lines}")

    def assert_message_received(self, response_lines, sender_nick, command, target, message_text=None):
        """
        Asserts that a specific message (e.g., PRIVMSG, NOTICE, JOIN, PART, KICK)
        was received, originating from a specific sender, with a specific command and target.
        Optionally checks for specific message text (for PRIVMSG, NOTICE, PART, KICK reasons).
        Example format: :<sender_nick>!<user>@<host> <COMMAND> <target> :<message_text>
        """
        expected_prefix = f":{sender_nick}!"
        expected_command_target = f" {command} {target}"
        found = False
        for line in response_lines:
            if line.startswith(expected_prefix) and expected_command_target in line:
                if message_text:
                    # For messages with trailing parameter, check for ":<message_text>"
                    self.assertIn(f":{message_text}", line, f"Expected message text '{message_text}' not found in message: {line}")
                found = True
                break
        self.assertTrue(found, f"Expected message from '{sender_nick}' with command '{command}' to '{target}' not found in response:\n{response_lines}")


if __name__ == '__main__':
    unittest.main()
