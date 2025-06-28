# tests/test_channel_modes1.py
import unittest
import socket
import time

# These imports come directly from irc_tester_utils.py
from irc_tester_utils import IrcTestBase, SERVER_HOST, SERVER_PORT, SERVER_PASSWORD, \
                             send_irc_command, recv_irc_response, \
                             RPL_WELCOME, RPL_CHANNELMODEIS, RPL_TOPIC, RPL_NOTOPIC, \
                             RPL_NAMREPLY, RPL_ENDOFNAMES, RPL_INVITING, \
                             ERR_NEEDMOREPARAMS, ERR_NOSUCHCHANNEL, ERR_UNKNOWNMODE, \
                             ERR_CHANOPRIVSNEEDED, ERR_CHANNELISFULL, ERR_INVITEONLYCHAN, \
                             ERR_BANNEDFROMCHAN, ERR_BADCHANNELKEY, ERR_USERNOTINCHANNEL, \
                             ERR_NOTONCHANNEL, ERR_UMODEUNKNOWNFLAG, ERR_NOSUCHNICK # Ensure ERR_NOSUCHNICK is imported


class TestChannelModes(IrcTestBase):
    """
    Test suite for IRC channel modes (+i, +k, +t, +l, +o).
    """

    def _register_aux_client(self, sock, nick_proposal, user="user", realname="Real Name"):
        """
        Helper method to register an auxiliary client on a specific socket.
        It handles initial PING/PONG and sends PASS, NICK, USER commands.
        Returns the actual nickname assigned by the server (from RPL_WELCOME)
        and the full response received.
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
        send_irc_command(sock, f"USER {user} 0 * :{realname}")
        
        response = recv_irc_response(sock, timeout=2.0)
        
        # Extract the actual assigned nickname from RPL_WELCOME (001) or other relevant message
        assigned_nick = nick_proposal # Default to proposed, if not found
        for line in response:
            if f":localhost {RPL_WELCOME}" in line:
                parts = line.split()
                if len(parts) > 2:
                    assigned_nick = parts[2] # Nickname is usually the 3rd token
                    break
            elif "NICK" in line and nick_proposal in line:
                # Fallback for NICK changes if no 001, though 001 is standard
                # This might be needed if server sends NICK message immediately after initial registration
                # and before RPL_WELCOME, or if the test setup is slightly different.
                pass # The register_client in IrcTestBase already updates self.current_nick.
                     # For aux clients, we need to manually track or infer.
        
        # Ensure RPL_WELCOME is received to confirm successful registration
        self.assert_irc_reply(response, RPL_WELCOME)
        
        print(f"DEBUG: Client proposed '{nick_proposal}', server assigned '{assigned_nick}'. Full response: {response}")
        return assigned_nick, response

    def test_mode_invite_only(self):
        """
        Test setting/unsetting invite-only mode (+i) and its effects.
        """
        op_nick_proposal = "OpUser"
        invited_nick_proposal = "InvitedUser"
        non_invited_nick_proposal = "NonInvited"
        channel_name = "#inviteonly"

        # 1. Op creates and joins the channel
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "opuser", "Op User")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages

        # 2. Op sets channel to invite-only (+i)
        print(f"Test: {op_actual_nick} setting {channel_name} to +i...")
        send_irc_command(op_sock, f"MODE {channel_name} +i")
        op_mode_set_response = recv_irc_response(op_sock)
        # Server should send RPL_CHANNELMODEIS (324) and echo the MODE command
        self.assert_irc_reply(op_mode_set_response, RPL_CHANNELMODEIS, f"{channel_name} +i")
        self.assert_message_received(op_mode_set_response, op_actual_nick, "MODE", f"{channel_name} +i")
        print(f"Test: {channel_name} is now +i.")

        # 3. Non-invited user tries to join (should fail with ERR_INVITEONLYCHAN)
        non_invited_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        non_invited_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(non_invited_sock.close)
        non_invited_actual_nick, _ = self._register_aux_client(non_invited_sock, non_invited_nick_proposal, "noninv", "Non-Invited")
        print(f"Test: {non_invited_actual_nick} attempting to join {channel_name} (should fail)...")
        send_irc_command(non_invited_sock, f"JOIN {channel_name}")
        non_invited_join_response = recv_irc_response(non_invited_sock)
        self.assert_irc_reply(non_invited_join_response, ERR_INVITEONLYCHAN, channel_name)
        self.assert_no_irc_reply(non_invited_join_response, RPL_WELCOME) # Should not get welcome
        print(f"Test: {non_invited_actual_nick} correctly denied access (ERR_INVITEONLYCHAN).")

        # 4. Op invites InvitedUser
        invited_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        invited_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(invited_sock.close)
        invited_actual_nick, _ = self._register_aux_client(invited_sock, invited_nick_proposal, "invitedu", "Invited User")
        print(f"Test: {op_actual_nick} inviting {invited_actual_nick} to {channel_name}...")
        send_irc_command(op_sock, f"INVITE {invited_actual_nick} {channel_name}")
        op_invite_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_invite_response, RPL_INVITING, f"{invited_actual_nick} {channel_name}")
        
        invited_receive_invite = recv_irc_response(invited_sock)
        self.assert_message_received(invited_receive_invite, op_actual_nick, "INVITE", invited_actual_nick, f":{channel_name}")
        print(f"Test: {invited_actual_nick} received invite.")

        # 5. Invited user joins after invite (should succeed)
        print(f"Test: {invited_actual_nick} attempting to join {channel_name} after invite (should succeed)...")
        send_irc_command(invited_sock, f"JOIN {channel_name}")
        invited_join_response = recv_irc_response(invited_sock)
        self.assert_message_received(invited_join_response, invited_actual_nick, "JOIN", channel_name)
        self.assert_irc_reply(invited_join_response, RPL_NAMREPLY, invited_actual_nick) # Should see self in names
        print(f"Test: {invited_actual_nick} successfully joined {channel_name}.")

        # 6. Op unsets invite-only mode (-i)
        print(f"Test: {op_actual_nick} unsetting {channel_name} -i...")
        send_irc_command(op_sock, f"MODE {channel_name} -i")
        op_mode_unset_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_mode_unset_response, RPL_CHANNELMODEIS, f"{channel_name} +") # Should reflect no +i
        self.assert_message_received(op_mode_unset_response, op_actual_nick, "MODE", f"{channel_name} -i")
        print(f"Test: {channel_name} is no longer +i.")

        # 7. Non-invited user tries to join again (should now succeed)
        print(f"Test: {non_invited_actual_nick} attempting to join {channel_name} again (should succeed)...")
        send_irc_command(non_invited_sock, f"JOIN {channel_name}")
        non_invited_join_response_2 = recv_irc_response(non_invited_sock)
        self.assert_message_received(non_invited_join_response_2, non_invited_actual_nick, "JOIN", channel_name)
        self.assert_irc_reply(non_invited_join_response_2, RPL_NAMREPLY, non_invited_actual_nick)
        print(f"Test: {non_invited_actual_nick} successfully joined {channel_name} after -i.")

    def test_mode_key(self):
        """
        Test setting/unsetting channel key mode (+k) and its effects.
        """
        op_nick_proposal = "KeyOp"
        valid_key_user_nick_proposal = "KeyUser"
        wrong_key_user_nick_proposal = "WrongKeyUser"
        no_key_user_nick_proposal = "NoKeyUser"
        channel_name = "#keychan"
        channel_key = "secretpass"

        # 1. Op creates and joins the channel
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "keyop", "Key Op")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages

        # 2. Op sets channel with key (+k)
        print(f"Test: {op_actual_nick} setting {channel_name} to +k {channel_key}...")
        send_irc_command(op_sock, f"MODE {channel_name} +k {channel_key}")
        op_mode_set_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_mode_set_response, RPL_CHANNELMODEIS, f"{channel_name} +k") # Should contain +k, might not show key
        self.assert_message_received(op_mode_set_response, op_actual_nick, "MODE", f"{channel_name} +k {channel_key}")
        print(f"Test: {channel_name} is now +k with key '{channel_key}'.")

        # 3. User tries to join without key (should fail with ERR_BADCHANNELKEY)
        no_key_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        no_key_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(no_key_sock.close)
        no_key_actual_nick, _ = self._register_aux_client(no_key_sock, no_key_user_nick_proposal, "nokey", "No Key User")
        print(f"Test: {no_key_actual_nick} attempting to join {channel_name} without key (should fail)...")
        send_irc_command(no_key_sock, f"JOIN {channel_name}")
        no_key_join_response = recv_irc_response(no_key_sock)
        self.assert_irc_reply(no_key_join_response, ERR_BADCHANNELKEY, channel_name)
        print(f"Test: {no_key_actual_nick} correctly denied access (ERR_BADCHANNELKEY).")

        # 4. User tries to join with wrong key (should fail with ERR_BADCHANNELKEY)
        wrong_key_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        wrong_key_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(wrong_key_sock.close)
        wrong_key_actual_nick, _ = self._register_aux_client(wrong_key_sock, wrong_key_user_nick_proposal, "wrongkey", "Wrong Key User")
        print(f"Test: {wrong_key_actual_nick} attempting to join {channel_name} with wrong key (should fail)...")
        send_irc_command(wrong_key_sock, f"JOIN {channel_name} wrong_pass")
        wrong_key_join_response = recv_irc_response(wrong_key_sock)
        self.assert_irc_reply(wrong_key_join_response, ERR_BADCHANNELKEY, channel_name)
        print(f"Test: {wrong_key_actual_nick} correctly denied access (ERR_BADCHANNELKEY).")

        # 5. User tries to join with correct key (should succeed)
        valid_key_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        valid_key_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(valid_key_sock.close)
        valid_key_actual_nick, _ = self._register_aux_client(valid_key_sock, valid_key_user_nick_proposal, "validkey", "Valid Key User")
        print(f"Test: {valid_key_actual_nick} attempting to join {channel_name} with correct key (should succeed)...")
        send_irc_command(valid_key_sock, f"JOIN {channel_name} {channel_key}")
        valid_key_join_response = recv_irc_response(valid_key_sock)
        self.assert_message_received(valid_key_join_response, valid_key_actual_nick, "JOIN", channel_name)
        self.assert_irc_reply(valid_key_join_response, RPL_NAMREPLY, valid_key_actual_nick)
        print(f"Test: {valid_key_actual_nick} successfully joined {channel_name}.")

        # 6. Op unsets key mode (-k)
        print(f"Test: {op_actual_nick} unsetting {channel_name} -k...")
        send_irc_command(op_sock, f"MODE {channel_name} -k")
        op_mode_unset_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_mode_unset_response, RPL_CHANNELMODEIS, f"{channel_name} +") # Should reflect no +k
        self.assert_message_received(op_mode_unset_response, op_actual_nick, "MODE", f"{channel_name} -k")
        print(f"Test: {channel_name} is no longer +k.")

        # 7. No key user tries to join again (should now succeed)
        print(f"Test: {no_key_actual_nick} attempting to join {channel_name} again (should succeed)...")
        send_irc_command(no_key_sock, f"JOIN {channel_name}")
        no_key_join_response_2 = recv_irc_response(no_key_sock)
        self.assert_message_received(no_key_join_response_2, no_key_actual_nick, "JOIN", channel_name)
        self.assert_irc_reply(no_key_join_response_2, RPL_NAMREPLY, no_key_actual_nick)
        print(f"Test: {no_key_actual_nick} successfully joined {channel_name} after -k.")

    def test_mode_topic_op_only(self):
        """
        Test setting topic operator-only mode (+t) and its effects.
        """
        op_nick_proposal = "TopicOp"
        regular_nick_proposal = "RegularTopicUser"
        channel_name = "#topiconly"
        initial_topic = "This is the initial topic."
        new_topic_by_op = "Operator changed this topic."
        new_topic_by_regular = "Regular user changed this topic."

        # 1. Op creates and joins the channel, sets initial topic
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "topop", "Topic Op")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages
        send_irc_command(op_sock, f"TOPIC {channel_name} :{initial_topic}")
        recv_irc_response(op_sock) # Clear topic set responses

        # 2. Op sets channel to topic settable by operators only (+t)
        print(f"Test: {op_actual_nick} setting {channel_name} to +t...")
        send_irc_command(op_sock, f"MODE {channel_name} +t")
        op_mode_set_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_mode_set_response, RPL_CHANNELMODEIS, f"{channel_name} +t")
        self.assert_message_received(op_mode_set_response, op_actual_nick, "MODE", f"{channel_name} +t")
        print(f"Test: {channel_name} is now +t.")

        # 3. Regular user joins the channel
        regular_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        regular_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(regular_sock.close)
        regular_actual_nick, _ = self._register_aux_client(regular_sock, regular_nick_proposal, "regulart", "Regular User")
        send_irc_command(regular_sock, f"JOIN {channel_name}")
        recv_irc_response(regular_sock) # Clear join messages

        # 4. Op changes topic (should succeed)
        print(f"Test: {op_actual_nick} changing topic (should succeed)...")
        send_irc_command(op_sock, f"TOPIC {channel_name} :{new_topic_by_op}")
        op_topic_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_topic_response, RPL_TOPIC, new_topic_by_op)
        # All channel members should see the topic change
        regular_sees_topic = recv_irc_response(regular_sock)
        self.assert_message_received(regular_sees_topic, op_actual_nick, "TOPIC", channel_name, new_topic_by_op)
        print(f"Test: {op_actual_nick} successfully changed topic.")

        # 5. Regular user tries to change topic (should fail with ERR_CHANOPRIVSNEEDED)
        print(f"Test: {regular_actual_nick} attempting to change topic (should fail)...")
        send_irc_command(regular_sock, f"TOPIC {channel_name} :{new_topic_by_regular}")
        regular_topic_response = recv_irc_response(regular_sock)
        self.assert_irc_reply(regular_topic_response, ERR_CHANOPRIVSNEEDED, channel_name)
        print(f"Test: {regular_actual_nick} correctly denied topic change (ERR_CHANOPRIVSNEEDED).")

        # 6. Op unsets topic operator-only mode (-t)
        print(f"Test: {op_actual_nick} unsetting {channel_name} -t...")
        send_irc_command(op_sock, f"MODE {channel_name} -t")
        op_mode_unset_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_mode_unset_response, RPL_CHANNELMODEIS, f"{channel_name} +") # Should reflect no +t
        self.assert_message_received(op_mode_unset_response, op_actual_nick, "MODE", f"{channel_name} -t")
        print(f"Test: {channel_name} is no longer +t.")

        # 7. Regular user tries to change topic again (should now succeed)
        print(f"Test: {regular_actual_nick} attempting to change topic again (should succeed)...")
        send_irc_command(regular_sock, f"TOPIC {channel_name} :{new_topic_by_regular}")
        regular_topic_response_2 = recv_irc_response(regular_sock)
        self.assert_irc_reply(regular_topic_response_2, RPL_TOPIC, new_topic_by_regular)
        op_sees_topic_2 = recv_irc_response(op_sock)
        self.assert_message_received(op_sees_topic_2, regular_actual_nick, "TOPIC", channel_name, new_topic_by_regular)
        print(f"Test: {regular_actual_nick} successfully changed topic after -t.")

    def test_mode_user_limit(self):
        """
        Test setting/unsetting user limit mode (+l) and its effects.
        """
        op_nick_proposal = "LimitOp"
        first_user_nick_proposal = "FirstUser"
        second_user_nick_proposal = "SecondUser"
        channel_name = "#limitchan"
        limit_count = 1

        # 1. Op creates and joins the channel
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "limitop", "Limit Op")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages

        # 2. Op sets channel with user limit (+l)
        print(f"Test: {op_actual_nick} setting {channel_name} to +l {limit_count}...")
        send_irc_command(op_sock, f"MODE {channel_name} +l {limit_count}")
        op_mode_set_response = recv_irc_response(op_sock)
        # RPL_CHANNELMODEIS might not include the limit value itself in the mode string, just '+l'
        self.assert_irc_reply(op_mode_set_response, RPL_CHANNELMODEIS, f"{channel_name} +l") 
        self.assert_message_received(op_mode_set_response, op_actual_nick, "MODE", f"{channel_name} +l {limit_count}")
        print(f"Test: {channel_name} is now +l {limit_count}.")

        # 3. First user joins (should succeed as it's within limit)
        first_user_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        first_user_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(first_user_sock.close)
        first_user_actual_nick, _ = self._register_aux_client(first_user_sock, first_user_nick_proposal, "firstu", "First User")
        print(f"Test: {first_user_actual_nick} attempting to join {channel_name} (should succeed)...")
        send_irc_command(first_user_sock, f"JOIN {channel_name}")
        first_user_join_response = recv_irc_response(first_user_sock)
        self.assert_message_received(first_user_join_response, first_user_actual_nick, "JOIN", channel_name)
        self.assert_irc_reply(first_user_join_response, RPL_NAMREPLY, first_user_actual_nick)
        print(f"Test: {first_user_actual_nick} successfully joined {channel_name}.")

        # 4. Second user tries to join (should fail with ERR_CHANNELISFULL)
        second_user_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        second_user_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(second_user_sock.close)
        second_user_actual_nick, _ = self._register_aux_client(second_user_sock, second_user_nick_proposal, "secondu", "Second User")
        print(f"Test: {second_user_actual_nick} attempting to join {channel_name} (should fail - channel full)...")
        send_irc_command(second_user_sock, f"JOIN {channel_name}")
        second_user_join_response = recv_irc_response(second_user_sock)
        self.assert_irc_reply(second_user_join_response, ERR_CHANNELISFULL, channel_name)
        print(f"Test: {second_user_actual_nick} correctly denied access (ERR_CHANNELISFULL).")

        # 5. Op unsets user limit mode (-l)
        print(f"Test: {op_actual_nick} unsetting {channel_name} -l...")
        send_irc_command(op_sock, f"MODE {channel_name} -l")
        op_mode_unset_response = recv_irc_response(op_sock)
        self.assert_irc_reply(op_mode_unset_response, RPL_CHANNELMODEIS, f"{channel_name} +") # Should reflect no +l
        self.assert_message_received(op_mode_unset_response, op_actual_nick, "MODE", f"{channel_name} -l")
        print(f"Test: {channel_name} is no longer +l.")

        # 6. Second user tries to join again (should now succeed)
        print(f"Test: {second_user_actual_nick} attempting to join {channel_name} again (should succeed)...")
        send_irc_command(second_user_sock, f"JOIN {channel_name}")
        second_user_join_response_2 = recv_irc_response(second_user_sock)
        self.assert_message_received(second_user_join_response_2, second_user_actual_nick, "JOIN", channel_name)
        self.assert_irc_reply(second_user_join_response_2, RPL_NAMREPLY, second_user_actual_nick)
        print(f"Test: {second_user_actual_nick} successfully joined {channel_name} after -l.")

    def test_mode_operator_grant_revoke(self):
        """
        Test granting and revoking channel operator status (+o / -o).
        """
        op_nick_proposal = "AdminOp"
        target_nick_proposal = "TargetUser"
        channel_name = "#opchan"

        # 1. Op creates and joins the channel
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "adminop", "Admin Op")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages

        # 2. Target user joins the channel
        target_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        target_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(target_sock.close)
        target_actual_nick, _ = self._register_aux_client(target_sock, target_nick_proposal, "targetu", "Target User")
        send_irc_command(target_sock, f"JOIN {channel_name}")
        recv_irc_response(target_sock) # Clear join messages

        # 3. Op grants operator status to TargetUser (+o)
        print(f"Test: {op_actual_nick} granting +o to {target_actual_nick} in {channel_name}...")
        send_irc_command(op_sock, f"MODE {channel_name} +o {target_actual_nick}")
        op_mode_set_response = recv_irc_response(op_sock)
        # The MODE command itself should be echoed
        self.assert_message_received(op_mode_set_response, op_actual_nick, "MODE", f"{channel_name} +o {target_actual_nick}")
        # Target user should receive the mode change notification
        target_sees_mode_change = recv_irc_response(target_sock)
        self.assert_message_received(target_sees_mode_change, op_actual_nick, "MODE", f"{channel_name} +o {target_actual_nick}")
        print(f"Test: {target_actual_nick} is now an operator.")

        # 4. Verify TargetUser can now perform operator actions (e.g., set invite-only mode)
        print(f"Test: {target_actual_nick} (new op) attempting to set +i (should succeed)...")
        send_irc_command(target_sock, f"MODE {channel_name} +i")
        target_mode_set_response = recv_irc_response(target_sock)
        self.assert_irc_reply(target_mode_set_response, RPL_CHANNELMODEIS, f"{channel_name} +i")
        self.assert_message_received(target_mode_set_response, target_actual_nick, "MODE", f"{channel_name} +i")
        print(f"Test: {target_actual_nick} successfully set +i mode.")

        # 5. Op revokes operator status from TargetUser (-o)
        print(f"Test: {op_actual_nick} revoking -o from {target_actual_nick} in {channel_name}...")
        send_irc_command(op_sock, f"MODE {channel_name} -o {target_actual_nick}")
        op_mode_unset_response = recv_irc_response(op_sock)
        self.assert_message_received(op_mode_unset_response, op_actual_nick, "MODE", f"{channel_name} -o {target_actual_nick}")
        # Target user should receive the mode change notification
        target_sees_mode_change_2 = recv_irc_response(target_sock)
        self.assert_message_received(target_sees_mode_change_2, op_actual_nick, "MODE", f"{channel_name} -o {target_actual_nick}")
        print(f"Test: {target_actual_nick} is no longer an operator.")

        # 6. Verify TargetUser can no longer perform operator actions (e.g., set invite-only mode)
        print(f"Test: {target_actual_nick} (ex-op) attempting to set +i (should fail)...")
        send_irc_command(target_sock, f"MODE {channel_name} +i")
        target_mode_set_response_2 = recv_irc_response(target_sock)
        self.assert_irc_reply(target_mode_set_response_2, ERR_CHANOPRIVSNEEDED, channel_name)
        print(f"Test: {target_actual_nick} correctly denied setting +i mode (ERR_CHANOPRIVSNEEDED).")

    def test_mode_unknown_mode_flag(self):
        """
        Test attempting to set an unknown mode flag.
        """
        op_nick_proposal = "UnknownModeTester"
        channel_name = "#unknownmode"
        unknown_mode_flag = "z" # An unlikely valid mode character

        # 1. Op creates and joins the channel
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "unknownu", "Unknown Mode User")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages

        # 2. Op attempts to set an unknown mode
        print(f"Test: {op_actual_nick} attempting to set unknown mode +{unknown_mode_flag} on {channel_name}...")
        send_irc_command(op_sock, f"MODE {channel_name} +{unknown_mode_flag}")
        response = recv_irc_response(op_sock)
        
        self.assert_irc_reply(response, ERR_UNKNOWNMODE, f"{unknown_mode_flag}")
        print(f"Test: {op_actual_nick} correctly received ERR_UNKNOWNMODE.")

    def test_mode_missing_parameters_for_o(self):
        """
        Test attempting to set +o mode without providing a nickname.
        """
        op_nick_proposal = "ParamTester"
        channel_name = "#noparamchan"

        # 1. Op creates and joins the channel
        op_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        op_sock.connect((self.SERVER_HOST, self.SERVER_PORT))
        self.addCleanup(op_sock.close)
        op_actual_nick, _ = self._register_aux_client(op_sock, op_nick_proposal, "paramu", "Param User")
        send_irc_command(op_sock, f"JOIN {channel_name}")
        recv_irc_response(op_sock) # Clear join messages

        # 2. Op attempts to set +o mode without a nickname
        print(f"Test: {op_actual_nick} attempting to set +o without target nick...")
        send_irc_command(op_sock, f"MODE {channel_name} +o")
        response = recv_irc_response(op_sock)
        
        self.assert_irc_reply(response, ERR_NEEDMOREPARAMS, "MODE")
        print(f"Test: {op_actual_nick} correctly received ERR_NEEDMOREPARAMS.")

if __name__ == '__main__':
    unittest.main(exit=False)
