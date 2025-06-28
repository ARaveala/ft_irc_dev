# tests/test_messaging.py
import unittest
import socket
from irc_tester_utils import IrcTestBase, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, ERR_NOSUCHNICK, ERR_NORECIPIENT, ERR_NOTEXTTOSEND # Removed unneeded imports

class TestMessaging(IrcTestBase):

    def test_privmsg_to_existing_user(self):
        """
        Tests sending a PRIVMSG to another online user.
        """
        # Register client A
        client_a_nick = "Alice"
        registration_responses_a = self.register_client(client_a_nick, "userA", "Alice User")
        self.assert_irc_reply(registration_responses_a, RPL_WELCOME) # Use constant for 001

        # Register client B (on a separate socket)
        sock_b = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_b.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sock_b.close) # Ensure socket is closed

        initial_ping_b = recv_irc_response(sock_b)
        for line in initial_ping_b:
            if line.startswith('PING'):
                send_irc_command(sock_b, line.replace('PING', 'PONG'))

        registration_responses_b = self._register_client_on_socket(sock_b, "Bob", "BobUser", "Bob User")
        self.assert_irc_reply(registration_responses_b, RPL_WELCOME) # Use constant for 001
        print(f"Test: Bob registered. Responses:\n{registration_responses_b}")

        # Alice sends PRIVMSG to Bob
        message = "Hello Bob, this is Alice!"
        send_irc_command(self.sock, f"PRIVMSG Bob :{message}")
        # Alice doesn't usually get a direct confirmation for PRIVMSG unless an error occurs,
        # so we primarily check Bob's reception.
        
        # Bob receives the message
        response_b_msg = recv_irc_response(sock_b, timeout=1.0)
        
        self.assert_message_received(response_b_msg, client_a_nick, "PRIVMSG", "Bob", message)
        print(f"Test: PRIVMSG to existing user successful. Responses:\n{response_b_msg}")


    def test_privmsg_to_nonexistent_user(self):
        """
        Tests sending a PRIVMSG to a user that does not exist.
        """
        registration_responses = self.register_client("SenderNick", "senderuser", "Sender User")
        self.assert_irc_reply(registration_responses, RPL_WELCOME) # Use constant for 001

        non_existent_nick = "NonExistentUser"
        message = "Are you there?"
        send_irc_command(self.sock, f"PRIVMSG {non_existent_nick} :{message}")
        response = recv_irc_response(self.sock)

        self.assert_irc_reply(response, ERR_NOSUCHNICK, non_existent_nick)
        print(f"Test: PRIVMSG to non-existent user rejected. Responses:\n{response}")

    def test_privmsg_no_recipient(self):
        """
        Tests sending a PRIVMSG without specifying a recipient.
        """
        registration_responses = self.register_client("SenderNick", "senderuser", "Sender User")
        self.assert_irc_reply(registration_responses, RPL_WELCOME) # Use constant for 001

        send_irc_command(self.sock, "PRIVMSG :Hello")
        response = recv_irc_response(self.sock)

        self.assert_irc_reply(response, ERR_NORECIPIENT, "No recipient given") # Server should respond with "No recipient given"
        print(f"Test: PRIVMSG with no recipient rejected. Responses:\n{response}")

    def test_privmsg_no_text_to_send(self):
        """
        Tests sending a PRIVMSG without any message text.
        """
        registration_responses = self.register_client("SenderNick", "senderuser", "Sender User")
        self.assert_irc_reply(registration_responses, RPL_WELCOME) # Use constant for 001

        send_irc_command(self.sock, "PRIVMSG SomeUser") # Missing trailing colon and text
        response = recv_irc_response(self.sock)

        self.assert_irc_reply(response, ERR_NOTEXTTOSEND, "No text to send") # Server should respond with "No text to send"
        print(f"Test: PRIVMSG with no text rejected. Responses:\n{response}")

    def test_notice_to_existing_user(self):
        """
        Tests sending a NOTICE to another online user. (NOTICE does not trigger automatic replies).
        """
        # Register client C
        client_c_nick = "Charlie"
        registration_responses_c = self.register_client(client_c_nick, "userC", "Charlie User")
        self.assert_irc_reply(registration_responses_c, RPL_WELCOME) # Use constant for 001

        # Register client D (on a separate socket)
        sock_d = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_d.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sock_d.close) # Ensure socket is closed

        initial_ping_d = recv_irc_response(sock_d)
        for line in initial_ping_d:
            if line.startswith('PING'):
                send_irc_command(sock_d, line.replace('PING', 'PONG'))

        registration_responses_d = self._register_client_on_socket(sock_d, "David", "DavidUser", "David User")
        self.assert_irc_reply(registration_responses_d, RPL_WELCOME) # Use constant for 001
        print(f"Test: David registered. Responses:\n{registration_responses_d}")

        # Charlie sends NOTICE to David
        message = "This is a notice for David from Charlie."
        send_irc_command(self.sock, f"NOTICE David :{message}")
        
        # David receives the message
        response_d_msg = recv_irc_response(sock_d, timeout=1.0)
        
        self.assert_message_received(response_d_msg, client_c_nick, "NOTICE", "David", message)
        print(f"Test: NOTICE to existing user successful. Responses:\n{response_d_msg}")


if __name__ == '__main__':
    unittest.main(exit=False)
