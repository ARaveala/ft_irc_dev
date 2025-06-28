# tests/test_basic_connection.py
import unittest
import socket
from irc_tester_utils import IrcTestBase, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, RPL_YOURHOST, RPL_CREATED, RPL_MYINFO, \
                             ERR_NOMOTD, ERR_PASSWDMISMATCH, ERR_NOTREGISTERED # Ensure ERR_NOTREGISTERED is defined in irc_tester_utils.py if used

class TestBasicConnection(IrcTestBase): # Corrected base class name

    def test_client_connects_and_receives_welcome(self):
        """
        Tests that a client can connect and receives initial welcome messages (001-004).
        """
        response_lines = self.register_client("test_client", "testuser", "Test Client")

        # Using constants for clarity and maintainability
        self.assert_irc_reply(response_lines, RPL_WELCOME, "Welcome to the IRC server") # Adjusted message part for your server's output
        self.assert_irc_reply(response_lines, RPL_YOURHOST, "Your host is localhost")
        self.assert_irc_reply(response_lines, RPL_CREATED, "This server was created today")
        self.assert_irc_reply(response_lines, RPL_MYINFO, "localhost 1.0 o o") # Adjusted message part

        motd_found = False
        for line in response_lines:
            if "375" in line or "372" in line or "376" in line: # Checks for MOTDSTART, MOTD, ENDOFMOTD
                motd_found = True
                break
        
        # If no MOTD messages are found, assert ERR_NOMOTD (422)
        if not motd_found:
            self.assert_irc_reply(response_lines, "422") # ERR_NOMOTD (Note: This constant was not in the irc_tester_utils.py I gave, you might need to add it: ERR_NOMOTD = "422")
        
        print(f"Test: Client successfully connected and registered. Responses:\n{response_lines}")

    def test_connect_with_invalid_password(self):
        """
        Tests that a client is denied connection with an incorrect password.
        """
        self.sock.close() # Close the default socket from setUp
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.SERVER_HOST, self.SERVER_PORT))

        send_irc_command(sock, "PASS wrong_password")
        send_irc_command(sock, "NICK badpass_nick")
        send_irc_command(sock, "USER badpass_user 0 * :Bad Password")
        
        response = recv_irc_response(sock, timeout=1.0)
        sock.close()
        
        self.assert_irc_reply(response, ERR_PASSWDMISMATCH, "Password incorrect") # Using constant
        print(f"Test: Client denied with invalid password. Responses:\n{response}")

    def test_connect_without_password_when_required(self):
        """
        Tests that a client is denied connection if no password is provided when required.
        """
        self.sock.close() # Close the default socket from setUp
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.SERVER_HOST, self.SERVER_PORT))

        send_irc_command(sock, "NICK nopass_nick")
        send_irc_command(sock, "USER nopass_user 0 * :No Password")
        
        response = recv_irc_response(sock, timeout=1.0)
        sock.close()
        
        # Using constants for clarity
        self.assertTrue(any(ERR_PASSWDMISMATCH in line or ERR_NOTREGISTERED in line for line in response),
                        f"Expected ERR_PASSWDMISMATCH ({ERR_PASSWDMISMATCH}) or ERR_NOTREGISTERED ({ERR_NOTREGISTERED}) not found: {response}")
        print(f"Test: Client denied without password. Responses:\n{response}")

if __name__ == '__main__':
    unittest.main()