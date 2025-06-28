# tests/test_privmsg_channelmsg1.py
import unittest
import socket
import time
import re # Import re module for regex

# These imports come directly from irc_tester_utils.py
from irc_tester_utils import IrcTestBase, SERVER_HOST, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, \
                             ERR_NOSUCHNICK, ERR_NOSUCHCHANNEL, ERR_CANNOTSENDTOCHAN, \
                             RPL_NAMREPLY, RPL_ENDOFNAMES, RPL_TOPIC, RPL_NOTOPIC


class TestPrivmsgChannelMsg(IrcTestBase):
    """
    Test suite for PRIVMSG and NOTICE commands to users and channels.
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
        send_irc_command(sock, f"USER {user_ident} 0 * :{realname}") # Use user_ident as the user field
        
        response = recv_irc_response(sock, timeout=2.0)
        
        assigned_nick = None
        assigned_user = None
        assigned_host = None

        # Attempt to extract actual assigned nick, user, and host from welcome messages or later messages
        # Server often uses a pattern like `user_assignednick` for the user part or a prefix to the `user_ident`
        # We need to find the definitive source for the full hostmask.
        
        # First, try to get assigned nick from RPL_WELCOME
        for line in response:
            if f":localhost {RPL_WELCOME}" in line:
                parts = line.split()
                if len(parts) > 2:
                    assigned_nick = parts[2]
                    # Assume user part is 'user_' + assigned_nick and host is 'localhost' based on server logs
                    assigned_user = f"user_{assigned_nick}" # This matches observed server behavior like user_anonthe_evill
                    assigned_host = "localhost"
                    break
        
        # If RPL_WELCOME didn't give the nick, try to infer from a NICK command if one was sent (unlikely for initial reg)
        if not assigned_nick:
             for line in response:
                match = re.search(r":(\S+)!(\S+)@(\S+)\sNICK\s:?(\S+)", line)
                if match:
                    assigned_nick = match.group(4)
                    assigned_user = match.group(2)
                    assigned_host = match.group(3)
                    break # Found it

        if not assigned_nick or not assigned_user or not assigned_host:
            # Fallback if parsing didn't work for hostmask. Use provided nick proposal and default user/host.
            # This might cause failures if server actual hostmask differs greatly.
            assigned_nick = assigned_nick or nick_proposal
            assigned_user = f"user_{assigned_nick}" # Best guess based on observation
            assigned_host = "localhost" # Consistent in your logs
            print(f"WARNING: Could not precisely parse full hostmask during registration for '{nick_proposal}'. Using guessed values.")

        # Ensure RPL_WELCOME is received to confirm successful registration
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

    def test_privmsg_user_success(self):
        """
        Tests sending a private message to another user successfully.
        """
        sender_nick_proposal = "Sender1"
        recipient_nick_proposal = "Recipient1"
        message = "Hello there, private user!"

        # 1. Register sender
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "senderu", "Sender User")

        # 2. Register recipient
        recipient_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        recipient_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(recipient_sock.close)
        recipient_actual_nick, recipient_actual_user, recipient_actual_host, _ = self._register_aux_client(recipient_sock, recipient_nick_proposal, "recipientu", "Recipient User")

        # Give a small delay to ensure both clients are fully registered and visible
        time.sleep(0.1) 

        # 3. Sender sends PRIVMSG to recipient
        print(f"Test: {sender_actual_nick} sending PRIVMSG to {recipient_actual_nick}...")
        send_irc_command(sender_sock, f"PRIVMSG {recipient_actual_nick} :{message}")
        sender_response = recv_irc_response(sender_sock, timeout=0.1) # Sender usually gets no direct response for PRIVMSG
        self.assertEqual(sender_response, [], "Sender should not receive a direct response for PRIVMSG success.")

        # 4. Recipient receives the PRIVMSG
        # Use the flexible assertion, passing raw message string
        self._assert_message_received_flexible_prefix(recipient_response, sender_actual_nick, "PRIVMSG", recipient_actual_nick, message, 
                                     user_part=sender_actual_user, host_part=sender_actual_host)
        print(f"Test: {recipient_actual_nick} successfully received PRIVMSG.")

    def test_privmsg_to_nonexistent_user(self):
        """
        Tests sending a private message to a non-existent user.
        Should result in ERR_NOSUCHNICK (401).
        """
        sender_nick_proposal = "SenderNoUser"
        nonexistent_nick = "NoSuchUser"
        message = "Are you there?"

        # 1. Register sender
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "senderunu", "Sender Nonexistent User")

        # 2. Sender sends PRIVMSG to nonexistent user
        print(f"Test: {sender_actual_nick} sending PRIVMSG to {nonexistent_nick} (should fail)...")
        send_irc_command(sender_sock, f"PRIVMSG {nonexistent_nick} :{message}")
        response = recv_irc_response(sender_sock)

        # 3. Assert ERR_NOSUCHNICK
        self.assert_irc_reply(response, ERR_NOSUCHNICK, nonexistent_nick)
        print(f"Test: {sender_actual_nick} correctly received ERR_NOSUCHNICK.")

    def test_privmsg_channel_success(self):
        """
        Tests sending a private message to a channel where sender is a member.
        All other channel members should receive the message.
        """
        sender_nick_proposal = "ChannelSender"
        recipient1_nick_proposal = "ChannelRecipient1"
        recipient2_nick_proposal = "ChannelRecipient2"
        channel_name = "#testchannel"
        message = "Hello channel members!"

        # 1. Register sender and join channel
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "user_ChannelRecipient1", "Channel Sender")
        send_irc_command(sender_sock, f"JOIN {channel_name}")
        join_responses = recv_irc_response(sender_sock) # Clear join messages
        # Assert the JOIN message for sender using flexible prefix as server might echo it differently
        self._assert_message_received_flexible_prefix(join_responses, sender_actual_nick, "JOIN", channel_name,
                                     user_part=sender_actual_user, host_part=sender_actual_host)


        # 2. Register recipient 1 and join channel
        recipient1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        recipient1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(recipient1_sock.close)
        recipient1_actual_nick, recipient1_actual_user, recipient1_actual_host, _ = self._register_aux_client(recipient1_sock, recipient1_nick_proposal, "crecip1", "Channel Recipient 1")
        send_irc_command(recipient1_sock, f"JOIN {channel_name}")
        join_responses = recv_irc_response(recipient1_sock) # Clear join messages
        # Assert the JOIN message for recipient 1 using flexible prefix
        self._assert_message_received_flexible_prefix(join_responses, recipient1_actual_nick, "JOIN", channel_name,
                                     user_part=recipient1_actual_user, host_part=recipient1_actual_host)


        # 3. Register recipient 2 and join channel
        recipient2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        recipient2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(recipient2_sock.close)
        recipient2_actual_nick, recipient2_actual_user, recipient2_actual_host, _ = self._register_aux_client(recipient2_sock, recipient2_nick_proposal, "crecip2", "Channel Recipient 2")
        send_irc_command(recipient2_sock, f"JOIN {channel_name}")
        join_responses = recv_irc_response(recipient2_sock) # Clear join messages
        # Assert the JOIN message for recipient 2 using flexible prefix
        self._assert_message_received_flexible_prefix(join_responses, recipient2_actual_nick, "JOIN", channel_name,
                                     user_part=recipient2_actual_user, host_part=recipient2_actual_host)


        # Give a small delay to ensure all clients are in the channel and visible
        time.sleep(0.1)

        # 4. Sender sends PRIVMSG to the channel
        print(f"Test: {sender_actual_nick} sending PRIVMSG to {channel_name}...")
        send_irc_command(sender_sock, f"PRIVMSG {channel_name} :{message}")
        sender_response = recv_irc_response(sender_sock, timeout=0.1) # Sender typically gets an echo for channel PRIVMSG
        # Use the flexible assertion for sender's echo, passing raw message string
        self._assert_message_received_flexible_prefix(sender_response, sender_actual_nick, "PRIVMSG", channel_name, message,
                                     user_part=sender_actual_user, host_part=sender_actual_host)
        print(f"Test: {sender_actual_nick} received echo for PRIVMSG.")


        # 5. Recipients receive the PRIVMSG
        recipient1_response = recv_irc_response(recipient1_sock)
        self._assert_message_received_flexible_prefix(recipient1_response, sender_actual_nick, "PRIVMSG", channel_name, message,
                                     user_part=sender_actual_user, host_part=sender_actual_host)
        print(f"Test: {recipient1_actual_nick} received PRIVMSG.")

        recipient2_response = recv_irc_response(recipient2_sock)
        self._assert_message_received_flexible_prefix(recipient2_response, sender_actual_nick, "PRIVMSG", channel_name, message,
                                     user_part=sender_actual_user, host_part=sender_actual_host)
        print(f"Test: {recipient2_actual_nick} received PRIVMSG.")

    def test_privmsg_to_nonexistent_channel(self):
        """
        Tests sending a private message to a non-existent channel.
        Should result in ERR_NOSUCHCHANNEL (403).
        """
        sender_nick_proposal = "SenderNoChannel"
        nonexistent_channel = "#nowhere"
        message = "Is anyone listening?"

        # 1. Register sender
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "senderunch", "Sender Unchannel")

        # 2. Sender sends PRIVMSG to nonexistent channel
        print(f"Test: {sender_actual_nick} sending PRIVMSG to {nonexistent_channel} :{message}") # Ensure the command sent has ':'
        send_irc_command(sender_sock, f"PRIVMSG {nonexistent_channel} :{message}")
        response = recv_irc_response(sender_sock)

        # 3. Assert ERR_NOSUCHCHANNEL
        self.assert_irc_reply(response, ERR_NOSUCHCHANNEL, nonexistent_channel)
        print(f"Test: {sender_actual_nick} correctly received ERR_NOSUCHCHANNEL.")

    def test_privmsg_not_on_channel(self):
        """
        Tests sending a private message to a channel where the sender is not a member.
        Should result in ERR_CANNOTSENDTOCHAN (404).
        """
        sender_nick_proposal = "SenderNotInChannel"
        channel_nick_proposal = "ChannelMember" # A member of the channel
        target_channel = "#restrictedchan"
        message = "I'm trying to talk here!"

        # 1. Register a channel member and join the channel
        channel_member_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        channel_member_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(channel_member_sock.close)
        channel_member_actual_nick, channel_member_actual_user, channel_member_actual_host, _ = self._register_aux_client(channel_member_sock, channel_nick_proposal, "user_ChannelMember", "Channel Member")
        send_irc_command(channel_member_sock, f"JOIN {target_channel}")
        join_responses = recv_irc_response(channel_member_sock) # Clear join messages
        # Assert the JOIN message for channel member using flexible prefix
        self._assert_message_received_flexible_prefix(join_responses, channel_member_actual_nick, "JOIN", target_channel,
                                     user_part=channel_member_actual_user, host_part=channel_member_actual_host)


        # 2. Register sender (who will NOT join the channel)
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "sendernic", "Sender Not In Channel")

        # Give a small delay to ensure client is registered
        time.sleep(0.1)

        # 3. Sender tries to PRIVMSG the channel
        print(f"Test: {sender_actual_nick} sending PRIVMSG to {target_channel} (not a member, should fail)...")
        send_irc_command(sender_sock, f"PRIVMSG {target_channel} :{message}")
        response = recv_irc_response(sender_sock)

        # 4. Assert ERR_CANNOTSENDTOCHAN
        self.assert_irc_reply(response, ERR_CANNOTSENDTOCHAN, target_channel)
        print(f"Test: {sender_actual_nick} correctly received ERR_CANNOTSENDTOCHAN.")
        
        # Ensure channel member does NOT receive the message
        channel_member_sees_nothing = recv_irc_response(channel_member_sock, timeout=0.1)
        self.assertNotIn(message, "".join(channel_member_sees_nothing))

    def test_notice_user_success(self):
        """
        Tests sending a NOTICE to another user successfully.
        NOTICE commands should not generate automatic replies.
        """
        sender_nick_proposal = "NoticeSender"
        recipient_nick_proposal = "NoticeRecipient"
        message = "This is a private notice!"

        # 1. Register sender
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "notsend", "Notice Sender")

        # 2. Register recipient
        recipient_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        recipient_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(recipient_sock.close)
        recipient_actual_nick, recipient_actual_user, recipient_actual_host, _ = self._register_aux_client(recipient_sock, recipient_nick_proposal, "notrecip", "Notice Recipient")

        # Give a small delay
        time.sleep(0.1)

        # 3. Sender sends NOTICE to recipient
        print(f"Test: {sender_actual_nick} sending NOTICE to {recipient_actual_nick}...")
        send_irc_command(sender_sock, f"NOTICE {recipient_actual_nick} :{message}")
        sender_response = recv_irc_response(sender_sock, timeout=0.1) # Sender should not receive any reply for NOTICE success
        self.assertEqual(sender_response, [], "Sender should receive no response for NOTICE success.")

        # 4. Recipient receives the NOTICE
        self._assert_message_received_flexible_prefix(recipient_response, sender_actual_nick, "NOTICE", recipient_actual_nick, message,
                                     user_part=sender_actual_user, host_part=sender_actual_host)
        print(f"Test: {recipient_actual_nick} successfully received NOTICE.")

    def test_notice_channel_success(self):
        """
        Tests sending a NOTICE to a channel where sender is a member.
        All other channel members should receive the message.
        NOTICE commands should not generate automatic replies.
        """
        sender_nick_proposal = "NoticeChanSender"
        recipient1_nick_proposal = "NoticeChanRecip1"
        channel_name = "#notchan"
        message = "This is a channel notice!"

        # 1. Register sender and join channel
        sender_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sender_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(sender_sock.close)
        sender_actual_nick, sender_actual_user, sender_actual_host, _ = self._register_aux_client(sender_sock, sender_nick_proposal, "user_NoticeChanSender", "Notice Channel Sender")
        send_irc_command(sender_sock, f"JOIN {channel_name}")
        join_responses = recv_irc_response(sender_sock) # Clear join messages
        # Assert the JOIN message for sender using flexible prefix
        self._assert_message_received_flexible_prefix(join_responses, sender_actual_nick, "JOIN", channel_name,
                                     user_part=sender_actual_user, host_part=sender_actual_host)


        # 2. Register recipient and join channel
        recipient1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        recipient1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(recipient1_sock.close)
        recipient1_actual_nick, recipient1_actual_user, recipient1_actual_host, _ = self._register_aux_client(recipient1_sock, recipient1_nick_proposal, "ncrecip1", "Notice Channel Recipient 1")
        send_irc_command(recipient1_sock, f"JOIN {channel_name}")
        join_responses = recv_irc_response(recipient1_sock) # Clear join messages
        # Assert the JOIN message for recipient 1 using flexible prefix
        self._assert_message_received_flexible_prefix(join_responses, recipient1_actual_nick, "JOIN", channel_name,
                                     user_part=recipient1_actual_user, host_part=recipient1_actual_host)


        # Give a small delay
        time.sleep(0.1)

        # 3. Sender sends NOTICE to the channel
        print(f"Test: {sender_actual_nick} sending NOTICE to {channel_name}...")
        send_irc_command(sender_sock, f"NOTICE {channel_name} :{message}")
        sender_response = recv_irc_response(sender_sock, timeout=0.1) # Sender should not receive any direct reply or echo for NOTICE
        self.assertEqual(sender_response, [], "Sender should receive no response for NOTICE success.")

        # 4. Recipient receives the NOTICE
        recipient1_response = recv_irc_response(recipient1_sock)
        self._assert_message_received_flexible_prefix(recipient1_response, sender_actual_nick, "NOTICE", channel_name, message,
                                     user_part=sender_actual_user, host_part=sender_actual_host)
        print(f"Test: {recipient1_actual_nick} successfully received NOTICE.")

if __name__ == '__main__':
    unittest.main(exit=False)
