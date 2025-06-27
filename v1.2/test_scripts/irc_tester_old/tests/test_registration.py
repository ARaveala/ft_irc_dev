# tests/test_registration.py
import unittest
import socket 
from irc_tester_utils import IrcTestBase, SERVER_PORT, SERVER_PASSWORD, \
                             RPL_WELCOME, ERR_PASSWDMISMATCH, RPL_MOTDSTART, RPL_ENDOFMOTD, \
                             send_irc_command, recv_irc_response

class TestRegistration(IrcTestBase):

    def test_nick_change_after_registration(self):
        """
        Tests changing a nickname after successful registration.
        """
        initial_response = self.register_client("original_nick", "testuser", "Original User")
        self.assert_irc_reply(initial_response, "001")

        # Capture the actual nick the server registered the client with,
        # as this is what the server will use in the NICK message prefix.
        nick_before_change = self.current_nick

        new_nick = "new_and_improved"
        send_irc_command(self.sock, f"NICK {new_nick}")
        response = recv_irc_response(self.sock)

        # Assert NICK change confirmation: server sends a NICK message from the old nick
        # to the new nick.
        self.assert_message_received(response, nick_before_change, "NICK", new_nick)
        self.current_nick = new_nick # Update client's tracked nick after successful change
        print(f"Test: Nickname changed successfully to {new_nick}. Responses:\n{response}")

    def test_duplicate_nickname(self):
        """
        Tests attempting to use a nickname that is already taken.
        """
        client_a_nick = "Alice"
        self.register_client(client_a_nick, "userA", "Alice User")

        sock_b = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Use direct socket import
        sock_b.connect((self.SERVER_HOST, self.SERVER_PORT)) # Use self.SERVER_HOST/PORT
        initial_ping_b = recv_irc_response(sock_b)
        for line in initial_ping_b:
            if line.startswith('PING'):
                send_irc_command(sock_b, line.replace('PING', 'PONG'))

        send_irc_command(sock_b, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(sock_b, f"NICK {client_a_nick}")
        send_irc_command(sock_b, f"USER BobUser 0 * :Bob User")
        
        response_b = recv_irc_response(sock_b, timeout=1.0)
        sock_b.close()

        self.assert_irc_reply(response_b, "433", client_a_nick)
        print(f"Test: Duplicate nickname rejected. Responses:\n{response_b}")

    def test_nick_without_user_registration(self):
        """
        Tests sending only NICK without USER, verifying server state or error.
        """
        send_irc_command(self.sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(self.sock, "NICK just_nick")
        response = recv_irc_response(self.sock)
        
        self.assert_no_irc_reply(response, "001")
        print(f"Test: NICK without USER. Responses:\n{response}")
        
    def test_user_without_nick_registration(self):
        """
        Tests sending only USER without NICK, verifying server state or error.
        """
        send_irc_command(self.sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(self.sock, "USER just_user 0 * :Just User")
        response = recv_irc_response(self.sock)
        
        self.assert_no_irc_reply(response, "001")
        print(f"Test: USER without NICK. Responses:\n{response}")

    def test_incomplete_nick_command(self):
        """
        Tests sending a NICK command with missing parameters.
        """
        send_irc_command(self.sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(self.sock, "NICK")
        response = recv_irc_response(self.sock)
        self.assert_irc_reply(response, "431")
        print(f"Test: Incomplete NICK command. Responses:\n{response}")

    def test_incomplete_user_command(self):
        """
        Tests sending a USER command with missing parameters.
        """
        send_irc_command(self.sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(self.sock, "USER myuser")
        response = recv_irc_response(self.sock)
        self.assert_irc_reply(response, "461", "USER")
        print(f"Test: Incomplete USER command. Responses:\n{response}")

    def test_registration_order_independence(self):
        """
        Tests if NICK/USER can be sent in any order for full registration.
        """
        client_b_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Use direct socket import
        client_b_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        recv_irc_response(client_b_sock) # Handle PING

        send_irc_command(client_b_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client_b_sock, "USER order_user 0 * :Order User")
        send_irc_command(client_b_sock, "NICK order_nick")
        
        response_b = recv_irc_response(client_b_sock, timeout=2.0)
        client_b_sock.close()
        
        self.assert_irc_reply(response_b, "001")
        print(f"Test: Registration order independence (USER then NICK). Responses:\n{response_b}")

if __name__ == '__main__':
    unittest.main()