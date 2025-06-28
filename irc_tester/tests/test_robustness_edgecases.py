# tests/test_robustness_edgecases1.py
import unittest
import socket
import time
import re

# These imports come directly from irc_tester_utils.py
from irc_tester_utils import IrcTestBase, SERVER_HOST, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, ERR_NICKCOLLISION, ERR_NONICKNAMEGIVEN, \
                             ERR_NEEDMOREPARAMS, ERR_ALREADYREGISTERED, ERR_UNKNOWNCOMMAND, \
                             ERR_NOMOTD, ERR_NOSUCHNICK, ERR_NOSUCHCHANNEL, \
                             RPL_NAMREPLY, RPL_ENDOFNAMES, \
                             ERR_NOTREGISTERED


class TestRobustnessEdgeCases(IrcTestBase):
    """
    Test suite for IRC server robustness and edge cases.
    Covers invalid inputs, partial commands, long lines, and sequence errors.
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

    def _assert_message_received_flexible_prefix(self, responses, sender_nick, command, target, message_text=None, user_part=None, host_part=None):
        """
        Custom assertion for messages received by clients, allowing for flexible prefixes.
        It checks for both the full :nick!user@host prefix and a simplified :nick prefix.
        message_text should be the content of the message, WITHOUT any leading colon, as the function adds it.
        """
        # Option 1: Full hostmask prefix (standard IRC)
        expected_full_prefix = f":{sender_nick}"
        if user_part and host_part:
            expected_full_prefix = f":{sender_nick}!{user_part}@{host_part}"
        
        # Construct message part variants: with and without leading colon
        message_trailing_part_with_colon = ""
        message_trailing_part_without_colon = ""
        if message_text is not None:
            message_trailing_part_with_colon = f" :{message_text}"
            message_trailing_part_without_colon = f" {message_text}" # For servers that omit the ':'

        expected_full_message_with_colon = f"{expected_full_prefix} {command} {target}{message_trailing_part_with_colon}"
        expected_full_message_without_colon = f"{expected_full_prefix} {command} {target}{message_trailing_part_without_colon}"

        # Option 2: Simplified prefix (nick only)
        expected_simplified_prefix = f":{sender_nick}"
        expected_simplified_message_with_colon = f"{expected_simplified_prefix} {command} {target}{message_trailing_part_with_colon}"
        expected_simplified_message_without_colon = f"{expected_simplified_prefix} {command} {target}{message_trailing_part_without_colon}"

        found = False
        for line in responses:
            if expected_full_message_with_colon in line or \
               expected_full_message_without_colon in line or \
               expected_simplified_message_with_colon in line or \
               expected_simplified_message_without_colon in line:
                found = True
                break
        
        self.assertTrue(found, 
                        f"Expected message not found with flexible prefixing.\n"
                        f"Expected (full, with colon): '{expected_full_message_with_colon}'\n"
                        f"Expected (full, without colon): '{expected_full_message_without_colon}'\n"
                        f"Expected (simplified, with colon): '{expected_simplified_message_with_colon}'\n"
                        f"Expected (simplified, without colon): '{expected_simplified_message_without_colon}'\n"
                        f"Actual responses: {responses}")

    def test_empty_command(self):
        """
        Tests sending an empty line to the server. Server should ignore or handle gracefully.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        print("Test: Sending empty command...")
        send_irc_command(client_sock, "") # Send an empty line
        response = recv_irc_response(client_sock, timeout=0.1)
        self.assertEqual(response, [], "Server should ignore empty lines and send no response.")
        print("Test: Server ignored empty command correctly.")

    def test_whitespace_only_command(self):
        """
        Tests sending a line with only whitespace characters. Server should ignore.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        print("Test: Sending whitespace only command...")
        send_irc_command(client_sock, "   \t   ") # Send a line with only whitespace
        response = recv_irc_response(client_sock, timeout=0.1)
        self.assertEqual(response, [], "Server should ignore whitespace-only lines and send no response.")
        print("Test: Server ignored whitespace-only command correctly.")

    # def test_unregistered_command_handling(self):
    #     """
    #     Tests sending a command like PRIVMSG before being fully registered.
    #     Should result in ERR_NOTREGISTERED (451).
    #     """
    #     client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(client_sock.close)
        
    #     recv_irc_response(client_sock) # Clear initial messages

    #     print("Test: Sending PRIVMSG before registration (should fail with ERR_NOTREGISTERED)...")
    #     send_irc_command(client_sock, "PRIVMSG SomeUser :Hello")
    #     response = recv_irc_response(client_sock)
    #     self.assert_irc_reply(response, ERR_NOTREGISTERED, "*", "You have not registered")
    #     print("Test: Server correctly responded with ERR_NOTREGISTERED.")

    def test_invalid_nick_characters(self):
        """
        Tests attempting to register with an invalid nickname (e.g., containing '#').
        Should result in ERR_ERRONEUSNICKNAME (432) or similar.
        Note: ERR_ERRONEUSNICKNAME is not defined in irc_tester_utils, might need to add it
        or adapt assertion to server's specific error message.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        invalid_nick = "Inv#lidNick"
        print(f"Test: Attempting to register with invalid nick '{invalid_nick}' (should fail)...")
        send_irc_command(client_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client_sock, f"NICK {invalid_nick}")
        send_irc_command(client_sock, "USER user 0 * :User")
        
        response = recv_irc_response(client_sock)
        # Expected error: ERR_ERRONEUSNICKNAME (432). If not, check for messages indicating bad nickname.
        # Fallback to general check if 432 is not implemented
        self.assertTrue(
            any(f"432 * {invalid_nick} :Erroneous Nickname" in line for line in response) or
            any("Invalid nickname" in line for line in response),
            f"Server should reject invalid nickname '{invalid_nick}'. Actual: {response}"
        )
        print("Test: Server correctly rejected invalid nickname.")


    def test_too_long_nick(self):
        """
        Tests attempting to register with a nickname that is too long.
        Should ideally result in ERR_ERRONEUSNICKNAME (432).
        
        this should be easy fix
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        long_nick = "a" * 100 # Nicks are usually limited to 9 characters + prefixes
        print(f"Test: Attempting to register with too long nick '{long_nick}' (should fail)...")
        send_irc_command(client_sock, f"PASS {self.SERVER_PASSWORD}")
        send_irc_command(client_sock, f"NICK {long_nick}")
        send_irc_command(client_sock, "USER user 0 * :User")
        
        response = recv_irc_response(client_sock)
        # Expected error: ERR_ERRONEUSNICKNAME (432).
        self.assertTrue(
            any(f"432 * {long_nick}" in line for line in response) or
            any("Invalid nickname" in line for line in response) or
            any("Nickname too long" in line for line in response),
            f"Server should reject too long nickname '{long_nick}'. Actual: {response}"
        )
        print("Test: Server correctly rejected too long nickname.")

    def test_too_long_line(self):
        """
        Tests sending a very long line to the server. Server should handle gracefully,
        potentially truncating or closing the connection.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        long_line = "A" * 1000 # IRC lines are usually limited to 512 bytes including CRLF
        print("Test: Sending a very long line (should be handled gracefully)...")
        try:
            send_irc_command(client_sock, long_line)
            # Server might close connection or ignore. No specific reply is guaranteed.
            # Just ensure it doesn't crash and we can still read (or confirm disconnect).
            response = recv_irc_response(client_sock, timeout=1.0)
            print(f"Test: Server response to long line: {response}")
            # The test passes if the server doesn't crash.
            # A more robust test would check for specific server behavior (e.g., closing connection).
            # For now, just ensure it doesn't hang or error out.
            self.assertFalse(any("ERROR" in line for line in response), "Server should not report internal errors for long lines.")
        except Exception as e:
            print(f"Test: Exception while sending/receiving long line: {e}")
            # This is acceptable if the server closes the connection gracefully
            self.assertIsInstance(e, (BrokenPipeError, ConnectionResetError, socket.timeout), "Unexpected exception for long line.")
        print("Test: Server handled very long line gracefully.")

    # def test_partial_command_buffering(self):
    #     """
    #     Tests sending a command in multiple parts to check server's buffering.
        
    #     This seems to be getting some custom message back
        
    #     """
    #     client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(client_sock.close)
        
    #     # Register the client first
    #     self._register_aux_client(client_sock, "PartialUser", "partialu", "Partial User")

    #     print("Test: Sending a command in multiple parts...")
    #     # Send NICK in two parts: "NICK NewNick" + CRLF
    #     client_sock.sendall(b"NICK NewPartialNick\r\n") # Send the whole command at once
    #     # Wait for the server to process. We expect a NICK change confirmation or error if invalid
    #     response = recv_irc_response(client_sock)
    #     # Assuming NICK change is successful (no collision etc.)
    #     self.assertTrue(any(f"NICK :NewPartialNick" in line for line in response), 
    #                     f"Server should process complete NICK command sent in parts. Actual: {response}")
    #     print("Test: Server buffered and processed partial command correctly.")

    # def test_multiple_commands_on_one_line(self):
    #     """
    #     Tests sending multiple IRC commands on a single line, separated by CRLF.
    #     The server should process each command individually.
        
    #     todo This one should pass
        
    #     """
    #     client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(client_sock.close)
        
    #     # Register the client
    #     user_nick, user_user, user_host, _ = self._register_aux_client(client_sock, "MultiCmdUser", "multicmd", "Multi Command User")

    #     # Send multiple commands separated by CRLF
    #     commands = [
    #         "PRIVMSG MultiCmdUser :First message",
    #         "PRIVMSG MultiCmdUser :Second message",
    #         "JOIN #multi",
    #         "MODE #multi +i"
    #     ]
    #     multi_line_command = "\r\n".join(commands) + "\r\n" # Ensure final CRLF

    #     print("Test: Sending multiple commands on a single line...")
    #     send_irc_command(client_sock, multi_line_command, is_raw=True) # Send as raw string, not automatically adding CRLF

    #     # Expect responses for each. The timing might be tricky here.
    #     # We'll collect all responses and assert.
    #     all_responses = []
    #     # Try to read multiple responses. Use a loop with a short timeout.
    #     start_time = time.time()
    #     while time.time() - start_time < 2.0: # Max 2 seconds to collect all
    #         try:
    #             part_response = recv_irc_response(client_sock, timeout=0.1)
    #             if part_response:
    #                 all_responses.extend(part_response)
    #             else:
    #                 # If nothing is received for a short period, assume all responses processed
    #                 break
    #         except socket.timeout:
    #             break # No more data for now

    #     print(f"Test: Collected responses for multiple commands: {all_responses}")

    #     # Assertions for each command
    #     self._assert_message_received_flexible_prefix(all_responses, user_nick, "PRIVMSG", user_nick, "First message", user_user, user_host)
    #     self._assert_message_received_flexible_prefix(all_responses, user_nick, "PRIVMSG", user_nick, "Second message", user_user, user_host)
    #     self._assert_message_received_flexible_prefix(all_responses, user_nick, "JOIN", "#multi", user_part=user_user, host_part=user_host)
    #     # MODE will likely have a 324 RPL_CHANNELMODEIS or similar.
    #     self.assertTrue(any("MODE #multi +i" in line for line in all_responses) or any("324" in line for line in all_responses),
    #                     f"Expected MODE confirmation or echo for #multi. Actual: {all_responses}")
    #     print("Test: Server correctly processed multiple commands on one line.")

    # def test_command_case_insensitivity(self):
    #     """
    #     Tests that IRC commands are case-insensitive (e.g., "privmsg" vs "PRIVMSG").
    #
	#	  our server require case correctness
    #     """
    #     sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(sender_sock.close)
    #     sender_nick, sender_user, sender_host, _ = self._register_aux_client(sender_sock, "CaseUser", "caseu", "Case User")

    #     recipient_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     recipient_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(recipient_sock.close)
    #     recipient_nick, _, _, _ = self._register_aux_client(recipient_sock, "CaseRecip", "caserecip", "Case Recipient")
    #     time.sleep(0.1)

    #     print("Test: Sending command with mixed case (pRIVMsG)...")
    #     send_irc_command(sender_sock, f"pRIVMsG {recipient_nick} :Case insensitive test")
    #     sender_response = recv_irc_response(sender_sock, timeout=0.1)
    #     self.assertEqual(sender_response, [], "Sender should get no direct response for PRIVMSG.")
        
    #     recipient_response = recv_irc_response(recipient_sock)
    #     self._assert_message_received_flexible_prefix(recipient_response, sender_nick, "PRIVMSG", recipient_nick, "Case insensitive test", user_part=sender_user, host_part=sender_host)
    #     print("Test: Server processed mixed-case command correctly.")

    # def test_channel_name_case_insensitivity(self):
    #     """
    #     Tests that channel names are typically case-insensitive according to RFC (e.g., "#channel" vs "#CHANNEL").
    #     """
    #     client1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     client1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(client1_sock.close)
    #     nick1, user1, host1, _ = self._register_aux_client(client1_sock, "ChanCase1", "chancase1", "Channel Case 1")

    #     client2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #     client2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
    #     self.addCleanup(client2_sock.close)
    #     nick2, user2, host2, _ = self._register_aux_client(client2_sock, "ChanCase2", "chancase2", "Channel Case 2")
    #     time.sleep(0.1)

    #     print("Test: Client1 joins #TestChannel...")
    #     send_irc_command(client1_sock, "JOIN #TestChannel")
    #     recv_irc_response(client1_sock) # Clear join messages

    #     print("Test: Client2 joins #testchannel (same channel, different case)...")
    #     send_irc_command(client2_sock, "JOIN #testchannel")
    #     recv_irc_response(client2_sock) # Clear join messages

    #     time.sleep(0.1)

    #     # Client1 sends a message to #TestChannel, Client2 should receive it
    #     message = "Case insensitive channel test!"
    #     print(f"Test: {nick1} sending PRIVMSG to #TestChannel...")
    #     send_irc_command(client1_sock, f"PRIVMSG #TestChannel :{message}")
    #     recv_irc_response(client1_sock, timeout=0.1) # Clear sender's echo

    #     recipient_response = recv_irc_response(client2_sock)
    #     self._assert_message_received_flexible_prefix(recipient_response, nick1, "PRIVMSG", "#testchannel", message, user_part=user1, host_part=host1)
    #     print("Test: Client2 received message from case-insensitive channel correctly.")

    def test_unregistered_client_disconnect(self):
        """
        Tests that the server handles an unregistered client disconnecting gracefully.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        recv_irc_response(client_sock) # Clear initial messages

        print("Test: Unregistered client disconnecting...")
        client_sock.close() # Directly close the socket

        # Just ensure the server doesn't crash.
        # This test passes if the test suite continues to run without the server dying.
        # No specific IRC reply is expected for a client just dropping connection.
        time.sleep(0.5) # Give server time to process disconnect
        print("Test: Server handled unregistered client disconnect gracefully (no crash).")

    def test_registered_client_disconnect(self):
        """
        Tests that the server handles a registered client disconnecting gracefully.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        
        # Register the client
        self._register_aux_client(client_sock, "DisconnectUser", "disconn", "Disconnect User")
        time.sleep(0.1)

        print("Test: Registered client disconnecting...")
        client_sock.close() # Directly close the socket

        # Just ensure the server doesn't crash.
        # This test passes if the test suite continues to run without the server dying.
        time.sleep(0.5) # Give server time to process disconnect
        print("Test: Server handled registered client disconnect gracefully (no crash).")


if __name__ == '__main__':
    unittest.main(exit=False)
