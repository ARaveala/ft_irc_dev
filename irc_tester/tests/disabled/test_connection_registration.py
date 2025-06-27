# tests/test_connection_registration1.py
import unittest
import socket
import time
import re

# These imports come directly from irc_tester_utils.py
from irc_tester_utils import IrcTestBase, SERVER_HOST, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, ERR_NICKNAMEINUSE, ERR_NONICKNAMEGIVEN, \
                             ERR_NEEDMOREPARAMS, ERR_ALREADYREGISTERED, ERR_UNKNOWNCOMMAND, \
                             ERR_PASSWDMISMATCH, ERR_NOTREGISTERED


class TestConnectionRegistration(IrcTestBase):
    """
    Test suite for IRC client connection and registration.
    Covers successful registration, various NICK/USER/PASS scenarios,
    and handling of commands before/after registration.
    """

    def _register_aux_client(self, sock, nick_proposal, user_ident="user", realname="Real Name"):
        """
        Helper method to register an auxiliary client on a specific socket.
        It handles initial PING/PONG and sends PASS, NICK, USER commands.
        Returns the actual nickname assigned by the server (from RPL_WELCOME),
        the user part of the hostmask (e.g., "user_assignednick"), and the host part ("localhost").
        """
        # Ensure socket is not blocked waiting for previous data
        sock.setblocking(False)
        try:
            while True:
                data = sock.recv(4096)
                if not data:
                    break
        except BlockingIOError:
            pass # No data currently in buffer
        sock.setblocking(True) # Set back to blocking for `recv_irc_response`

        initial_response = recv_irc_response(sock)
        for line in initial_response:
            if line.startswith('PING'):
                pong_response = line.replace('PING', 'PONG')
                send_irc_command(sock, pong_response)
        
        send_irc_command(sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(sock, f"NICK {nick_proposal}")
        send_irc_command(sock, f"USER {user_ident} 0 * :{realname}")
        
        response = recv_irc_response(sock, timeout=2.0)
        
        assigned_nick = None
        assigned_user = None
        assigned_host = None

        # Attempt to extract actual assigned nick, user, and host from welcome messages or later messages
        for line in response:
            if f":localhost {RPL_WELCOME}" in line:
                parts = line.split()
                if len(parts) > 2:
                    assigned_nick = parts[2]
                    # Assume user part is 'user_' + assigned_nick and host is 'localhost' based on server logs
                    assigned_user = f"user_{assigned_nick}"
                    assigned_host = "localhost"
                    break
        
        if not assigned_nick:
             for line in response:
                match = re.search(r":(\S+)!(\S+)@(\S+)\sNICK\s:?(\S+)", line)
                if match:
                    assigned_nick = match.group(4)
                    assigned_user = match.group(2)
                    assigned_host = match.group(3)
                    break

        if not assigned_nick or not assigned_user or not assigned_host:
            assigned_nick = assigned_nick or nick_proposal
            assigned_user = f"user_{assigned_nick}"
            assigned_host = "localhost"
            print(f"WARNING: Could not precisely parse full hostmask during registration for '{nick_proposal}'. Using guessed values.")

        self.assert_irc_reply(response, RPL_WELCOME)
        
        print(f"DEBUG: Client proposed '{nick_proposal}', server assigned '{assigned_nick}' with user '{assigned_user}' and host '{assigned_host}'. Full response: {response}")
        return assigned_nick, assigned_user, assigned_host, response

    def test_nick_change_after_registration(self):
        """
        Tests changing nickname after successful registration.
        Expects a NICK message echo.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        old_nick, old_user, old_host, _ = self._register_aux_client(client_sock, "OldNick", "olduser", "Old User")
        time.sleep(0.1)

        new_nick = "NewNick"
        print(f"Test: {old_nick} changing nick to {new_nick}...")
        send_irc_command(client_sock, f"NICK {new_nick}")
        response = recv_irc_response(client_sock)

        # Server should send a NICK message with the new nick
        # Format: :old_nick!old_user@old_host NICK :new_nick
        expected_nick_change = f":{old_nick}!{old_user}@{old_host} NICK :{new_nick}"
        self.assertTrue(any(expected_nick_change in line for line in response),
                        f"Expected NICK change message not found. Actual: {response}")
        print("Test: Nick change after registration successful.")

    def test_nick_change_to_empty(self):
        """
        Tests changing nickname to an empty string.
        Should result in ERR_NONICKNAMEGIVEN (431).
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        self._register_aux_client(client_sock, "ValidNick", "validuser", "Valid User")
        time.sleep(0.1)

        print("Test: Attempting to change nick to empty (should fail)...")
        send_irc_command(client_sock, "NICK")
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, ERR_NONICKNAMEGIVEN)
        print("Test: Changing nick to empty correctly handled.")

    def test_user_command_after_registration(self):
        """
        Tests sending the USER command again after successful registration.
        Should result in ERR_ALREADYREGISTERED (462).
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        self._register_aux_client(client_sock, "RegUser", "reguser", "Registered User")
        time.sleep(0.1)

        print("Test: Sending USER command again after registration (should fail with ERR_ALREADYREGISTERED)...")
        send_irc_command(client_sock, "USER newuser 0 * :New User Info")
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, ERR_ALREADYREGISTERED)
        print("Test: USER command after registration correctly handled.")

    def test_nick_collision_during_registration(self):
        """
        Tests a client attempting to register with a nickname already in use.
        Should result in ERR_NICKNAMEINUSE (433).
        """
        # First client registers with a nick
        client1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client1_sock.close)
        
        used_nick = "UsedNick"
        self._register_aux_client(client1_sock, used_nick, "user1", "User One")
        time.sleep(0.1)

        # Second client attempts to register with the same nick
        client2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client2_sock.close)

        initial_response = recv_irc_response(client2_sock)
        for line in initial_response:
            if line.startswith('PING'):
                pong_response = line.replace('PING', 'PONG')
                send_irc_command(client2_sock, pong_response)

        print(f"Test: Client2 attempting to register with already used nick '{used_nick}' (should fail)...")
        send_irc_command(client2_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client2_sock, f"NICK {used_nick}") # Attempt to use the same nick
        send_irc_command(client2_sock, "USER user2 0 * :User Two")

        response = recv_irc_response(client2_sock)
        self.assert_irc_reply(response, ERR_NICKNAMEINUSE, used_nick)
        print("Test: Nick collision during registration correctly handled.")

    def test_nick_collision_after_registration(self):
        """
        Tests a registered client attempting to change to a nickname already in use.
        Should result in ERR_NICKNAMEINUSE (433).
        """
        # First client registers with a nick
        client1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client1_sock.close)
        
        used_nick = "AnotherUsedNick"
        self._register_aux_client(client1_sock, used_nick, "userA", "User A")
        time.sleep(0.1)

        # Second client registers with a different nick
        client2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client2_sock.close)
        
        client2_nick, client2_user, client2_host, _ = self._register_aux_client(client2_sock, "OriginalNick", "userB", "User B")
        time.sleep(0.1)

        print(f"Test: {client2_nick} attempting to change nick to used nick '{used_nick}' (should fail)...")
        send_irc_command(client2_sock, f"NICK {used_nick}") # Client2 tries to take used_nick
        response = recv_irc_response(client2_sock)
        self.assert_irc_reply(response, ERR_NICKNAMEINUSE, used_nick)
        print("Test: Nick collision after registration correctly handled.")

    def test_registration_order_user_first(self):
        """
        Tests registration when USER is sent before NICK.
        Should still result in successful registration once both are sent.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)

        initial_response = recv_irc_response(client_sock)
        for line in initial_response:
            if line.startswith('PING'):
                pong_response = line.replace('PING', 'PONG')
                send_irc_command(client_sock, pong_response)
        
        print("Test: Sending USER before NICK...")
        send_irc_command(client_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client_sock, "USER user_order 0 * :User Order Test")
        # No immediate replies expected for USER only, unless server requires NICK first for some temporary state.
        # However, the RFC states NICK and USER establish identity.
        time.sleep(0.1) # Small delay to let server process

        send_irc_command(client_sock, "NICK OrderedNick")
        
        response = recv_irc_response(client_sock)
        print(f"DEBUG: Responses for USER-first registration: {response}")
        self.assert_irc_reply(response, RPL_WELCOME, "OrderedNick")
        print("Test: USER-first registration successful.")

    def test_missing_user_params(self):
        """
        Tests sending USER command with missing parameters.
        Should result in ERR_NEEDMOREPARAMS (461).
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        print("Test: Sending USER with missing parameters (should fail)...")
        send_irc_command(client_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client_sock, "NICK MissingUserParams")
        send_irc_command(client_sock, "USER partialuser") # Missing 3 params
        
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, ERR_NEEDMOREPARAMS, "USER")
        print("Test: USER with missing parameters correctly handled.")

    def test_missing_nick_after_user(self):
        """
        Tests successful USER registration but missing NICK, then sending a command.
        Should result in ERR_NOTREGISTERED (451) for commands requiring full registration.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)

        initial_response = recv_irc_response(client_sock)
        for line in initial_response:
            if line.startswith('PING'):
                pong_response = line.replace('PING', 'PONG')
                send_irc_command(client_sock, pong_response)
        
        send_irc_command(client_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client_sock, "USER incomplete 0 * :Incomplete User")
        
        # Now try to send a command requiring full registration without a NICK
        print("Test: Sending PRIVMSG without NICK after USER (should fail with ERR_NOTREGISTERED)...")
        send_irc_command(client_sock, "PRIVMSG someuser :hello")
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, ERR_NOTREGISTERED)
        print("Test: PRIVMSG without full registration (missing NICK) correctly handled.")


if __name__ == '__main__':
    unittest.main(exit=False)
