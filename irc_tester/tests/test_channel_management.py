# tests/test_channel_management1.py
import unittest
import socket
import time
import re

# These imports come directly from irc_tester_utils.py
from irc_tester_utils import IrcTestBase, SERVER_HOST, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, RPL_NOTOPIC, RPL_TOPIC, RPL_NAMREPLY, RPL_ENDOFNAMES, \
                             RPL_INVITING, ERR_NOSUCHNICK, ERR_NOTONCHANNEL, ERR_CHANOPRIVSNEEDED, \
                             ERR_CHANNELISFULL, ERR_INVITEONLYCHAN, ERR_BADCHANNELKEY, \
                             ERR_NOSUCHCHANNEL, ERR_TOOMANYCHANNELS, ERR_UNKNOWNMODE


class TestChannelManagement(IrcTestBase):
    """
    Test suite for basic IRC channel management commands (JOIN, PART, TOPIC, INVITE).
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

    def test_join_new_channel_success(self):
        """
        Tests if a user can successfully join a new channel.
        Expects JOIN message, RPL_TOPIC (or RPL_NOTOPIC), and NAMES list.
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        client_nick, client_user, client_host, _ = self._register_aux_client(client_sock, "Joiner1", "joiner1", "Join User 1")

        channel_name = "#newchannel"
        print(f"Test: {client_nick} joining {channel_name}...")
        send_irc_command(client_sock, f"JOIN {channel_name}")
        response = recv_irc_response(client_sock)

        # Expect JOIN message echo back to client
        self._assert_message_received_flexible_prefix(response, client_nick, "JOIN", channel_name, user_part=client_user, host_part=client_host)
        self.assert_irc_reply(response, RPL_NOTOPIC, client_nick, channel_name) # Or RPL_TOPIC
        self.assert_irc_reply(response, RPL_NAMREPLY, client_nick, f"= {channel_name}")
        self.assert_irc_reply(response, RPL_ENDOFNAMES, client_nick, channel_name)
        print("Test: User joined new channel successfully.")

    def test_join_existing_channel_multiple_users(self):
        """
        Tests multiple users joining the same channel, and messages propagated correctly.
        """
        client1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client1_sock.close)
        nick1, user1, host1, _ = self._register_aux_client(client1_sock, "UserA", "usera", "User Alpha")

        channel_name = "#multiuser"
        send_irc_command(client1_sock, f"JOIN {channel_name}")
        recv_irc_response(client1_sock) # Clear join messages

        client2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client2_sock.close)
        nick2, user2, host2, _ = self._register_aux_client(client2_sock, "UserB", "userb", "User Beta")

        print(f"Test: {nick2} joining {channel_name} (existing)...")
        send_irc_command(client2_sock, f"JOIN {channel_name}")
        client2_response = recv_irc_response(client2_sock)
        client1_notification = recv_irc_response(client1_sock) # Client1 should see Client2's JOIN

        # Client2's assertions
        self._assert_message_received_flexible_prefix(client2_response, nick2, "JOIN", channel_name, user_part=user2, host_part=host2)
        self.assert_irc_reply(client2_response, RPL_NAMREPLY, nick2, f"= {channel_name}", expected_partial_match=f"{nick1}!{user1}@{host1} {nick2}!{user2}@{host2}") # Both users should be in NAMES list
        
        # Client1's assertion: should receive JOIN notification for Client2
        self._assert_message_received_flexible_prefix(client1_notification, nick2, "JOIN", channel_name, user_part=user2, host_part=host2)
        print("Test: Multiple users joined channel successfully.")

    def test_part_channel_success(self):
        """
        Tests if a user can successfully leave a channel.
        Expects PART message propagation.
        """
        client1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client1_sock.close)
        nick1, user1, host1, _ = self._register_aux_client(client1_sock, "PartUser1", "partuser1", "Part User 1")

        client2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client2_sock.close)
        nick2, user2, host2, _ = self._register_aux_client(client2_sock, "PartUser2", "partuser2", "Part User 2")

        channel_name = "#partchannel"
        send_irc_command(client1_sock, f"JOIN {channel_name}")
        send_irc_command(client2_sock, f"JOIN {channel_name}")
        recv_irc_response(client1_sock) # Clear join messages
        recv_irc_response(client2_sock) # Clear join messages
        time.sleep(0.1)

        print(f"Test: {nick1} parting {channel_name}...")
        part_message = "I'm out!"
        send_irc_command(client1_sock, f"PART {channel_name} :{part_message}")
        client1_part_response = recv_irc_response(client1_sock) # Should get PART echo
        client2_notification = recv_irc_response(client2_sock) # Client2 should see Client1's PART

        # Client1's assertion
        self._assert_message_received_flexible_prefix(client1_part_response, nick1, "PART", channel_name, part_message, user_part=user1, host_part=host1)
        
        # Client2's assertion: should receive PART notification for Client1
        self._assert_message_received_flexible_prefix(client2_notification, nick1, "PART", channel_name, part_message, user_part=user1, host_part=host1)
        print("Test: User parted channel successfully.")

    def test_part_nonexistent_channel(self):
        """
        Tests parting a channel that does not exist.
        Should result in ERR_NOSUCHCHANNEL (403).
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        client_nick, _, _, _ = self._register_aux_client(client_sock, "NoChanPart", "nochpart", "No Channel Part User")
        time.sleep(0.1)

        nonexistent_channel = "#nonexistent"
        print(f"Test: {client_nick} parting {nonexistent_channel} (should fail)...")
        send_irc_command(client_sock, f"PART {nonexistent_channel}")
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, ERR_NOSUCHCHANNEL, client_nick, nonexistent_channel)
        print("Test: Parting nonexistent channel correctly handled.")

    def test_part_not_on_channel(self):
        """
        Tests parting a channel that the user is not a member of.
        Should result in ERR_NOTONCHANNEL (442).
        """
        client1_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client1_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client1_sock.close)
        client1_nick, _, _, _ = self._register_aux_client(client1_sock, "NotOnChanPart", "notonpart", "Not On Channel Part User")

        # Create a channel with another user, so it exists
        client2_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client2_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client2_sock.close)
        self._register_aux_client(client2_sock, "OtherMember", "otherm", "Other Member")
        
        target_channel = "#otherchannel"
        send_irc_command(client2_sock, f"JOIN {target_channel}")
        recv_irc_response(client2_sock) # Clear join messages
        time.sleep(0.1)

        print(f"Test: {client1_nick} parting {target_channel} (not a member, should fail)...")
        send_irc_command(client1_sock, f"PART {target_channel}")
        response = recv_irc_response(client1_sock)
        self.assert_irc_reply(response, ERR_NOTONCHANNEL, client1_nick, target_channel)
        print("Test: Parting channel user is not on correctly handled.")

    def test_topic_get_no_topic(self):
        """
        Tests getting topic of a channel with no topic set.
        Should result in RPL_NOTOPIC (331).
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        client_nick, _, _, _ = self._register_aux_client(client_sock, "TopicGetter1", "topicget1", "Topic Getter 1")

        channel_name = "#notopicchan"
        send_irc_command(client_sock, f"JOIN {channel_name}")
        recv_irc_response(client_sock) # Clear join messages
        time.sleep(0.1)

        print(f"Test: {client_nick} getting topic of {channel_name} (no topic)...")
        send_irc_command(client_sock, f"TOPIC {channel_name}")
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, RPL_NOTOPIC, client_nick, channel_name)
        print("Test: Getting topic of channel with no topic correctly handled.")

    def test_topic_set_and_get(self):
        """
        Tests setting a topic for a channel and then retrieving it.
        Expects TOPIC message propagation and RPL_TOPIC (332) on retrieval.
        """
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_nick, op_user, op_host, _ = self._register_aux_client(op_sock, "TopicOp", "topicop", "Topic Operator")

        viewer_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        viewer_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(viewer_sock.close)
        viewer_nick, viewer_user, viewer_host, _ = self._register_aux_client(viewer_sock, "TopicViewer", "topicview", "Topic Viewer")

        channel_name = "#topicchan"
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages
        send_irc_command(viewer_sock, f"JOIN {channel_name}")
        recv_irc_response(viewer_sock) # Clear join messages
        time.sleep(0.1)

        new_topic = "This is the new channel topic!"
        print(f"Test: {op_nick} setting topic for {channel_name}: '{new_topic}'...")
        send_irc_command(op_sock, f"TOPIC {channel_name} :{new_topic}")
        # Op should receive TOPIC message echo
        op_topic_response = recv_irc_response(op_sock)
        self._assert_message_received_flexible_prefix(op_topic_response, op_nick, "TOPIC", channel_name, new_topic, user_part=op_user, host_part=op_host)

        # Viewer should receive TOPIC message notification
        viewer_topic_notification = recv_irc_response(viewer_sock)
        self._assert_message_received_flexible_prefix(viewer_topic_notification, op_nick, "TOPIC", channel_name, new_topic, user_part=op_user, host_part=op_host)
        print("Test: Topic set and propagated correctly.")

        # Now, viewer requests the topic
        print(f"Test: {viewer_nick} getting topic of {channel_name} (should be '{new_topic}')...")
        send_irc_command(viewer_sock, f"TOPIC {channel_name}")
        get_topic_response = recv_irc_response(viewer_sock)
        self.assert_irc_reply(get_topic_response, RPL_TOPIC, viewer_nick, channel_name, expected_partial_match=new_topic)
        print("Test: Topic retrieved correctly.")

    def test_invite_success(self):
        """
        Tests inviting a user to a channel.
        Expects RPL_INVITING (341) for inviter and INVITE message for invitee.
        """
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_nick, op_user, op_host, _ = self._register_aux_client(op_sock, "ChanOpForTest", "optest", "Operator User")

        inviter_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        inviter_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(inviter_sock.close)
        inviter_nick, inviter_user, inviter_host, _ = self._register_aux_client(inviter_sock, "RegularInviter", "regi", "Regular Inviter")

        invitee_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        invitee_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(invitee_sock.close)
        invitee_nick, invitee_user, invitee_host, _ = self._register_aux_client(invitee_sock, "InvitedOne", "invited", "Invited User")
        
        channel_name = "#optestchan"
        send_irc_command(op_sock, f"JOIN {channel_name}") # Op joins to create and be on channel
        recv_irc_response(op_sock)
        send_irc_command(inviter_sock, f"JOIN {channel_name}") # Inviter also joins
        recv_irc_response(inviter_sock)
        time.sleep(0.1)

        print(f"Test: {inviter_nick} inviting {invitee_nick} to {channel_name}...")
        send_irc_command(inviter_sock, f"INVITE {invitee_nick} {channel_name}")
        
        # Inviter should receive RPL_INVITING
        inviter_response = recv_irc_response(inviter_sock)
        self.assert_irc_reply(inviter_response, RPL_INVITING, inviter_nick, channel_name, expected_partial_match=invitee_nick)
        print(f"Test: {inviter_nick} received RPL_INVITING.")

        # Invitee should receive INVITE message
        invitee_response = recv_irc_response(invitee_sock)
        # INVITE message format: :<inviter_nick>!<user>@<host> INVITE <invitee_nick> :<channel>
        expected_invite_msg_prefix = f":{inviter_nick}!{inviter_user}@{inviter_host} INVITE {invitee_nick} :{channel_name}"
        self.assertTrue(any(expected_invite_msg_prefix in line for line in invitee_response),
                        f"Invitee did not receive expected INVITE message. Expected prefix: '{expected_invite_msg_prefix}'. Actual: {invitee_response}")
        print(f"Test: {invitee_nick} received INVITE message.")

    def test_invite_nonexistent_user(self):
        """
        Tests inviting a non-existent user.
        Should result in ERR_NOSUCHNICK (401).
        """
        client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(client_sock.close)
        client_nick, client_user, client_host, _ = self._register_aux_client(client_sock, "InviteSender", "inviter", "Invite Sender")
        
        channel_name = "#mychan"
        send_irc_command(client_sock, f"JOIN {channel_name}")
        recv_irc_response(client_sock) # Clear join messages
        time.sleep(0.1)

        nonexistent_nick = "GhostUser"
        print(f"Test: {client_nick} inviting {nonexistent_nick} (should fail)...")
        send_irc_command(client_sock, f"INVITE {nonexistent_nick} {channel_name}")
        response = recv_irc_response(client_sock)
        self.assert_irc_reply(response, ERR_NOSUCHNICK, client_nick, nonexistent_nick)
        print("Test: Inviting nonexistent user correctly handled.")

    def test_invite_not_on_channel(self):
        """
        Tests inviting a user to a channel when the inviter is not on that channel.
        Should result in ERR_NOTONCHANNEL (442).
        """
        inviter_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        inviter_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(inviter_sock.close)
        inviter_nick, _, _, _ = self._register_aux_client(inviter_sock, "InviterNotOnChan", "inviter_nc", "Inviter Not On Chan")

        invitee_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        invitee_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(invitee_sock.close)
        invitee_nick, _, _, _ = self._register_aux_client(invitee_sock, "InviteeForNC", "invitee_nc", "Invitee For Not On Chan")

        # Create the channel with another user, but inviter doesn't join
        channel_member_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        channel_member_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(channel_member_sock.close)
        self._register_aux_client(channel_member_sock, "CMemberForInvite", "cmember", "Channel Member for Invite")
        
        target_channel = "#inviteonly"
        send_irc_command(channel_member_sock, f"JOIN {target_channel}")
        recv_irc_response(channel_member_sock)
        time.sleep(0.1)

        print(f"Test: {inviter_nick} inviting {invitee_nick} to {target_channel} (inviter not on channel)...")
        send_irc_command(inviter_sock, f"INVITE {invitee_nick} {target_channel}")
        response = recv_irc_response(inviter_sock)
        self.assert_irc_reply(response, ERR_NOTONCHANNEL, inviter_nick, target_channel)
        print("Test: Inviting when not on channel correctly handled.")


if __name__ == '__main__':
    unittest.main(exit=False)
