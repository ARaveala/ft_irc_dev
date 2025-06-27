# tests/test_stability.py
import unittest
import socket
import time

# These imports come directly from irc_tester_utils.py
from irc_tester_utils import IrcTestBase, RPL_WELCOME, SERVER_HOST, SERVER_PORT, \
                             send_irc_command, recv_irc_response

class TestStability(IrcTestBase):
    """
    A dedicated test class to verify the continuous stability and responsiveness
    of the IRC server.
    """

    def test_server_maintains_connection_and_responds(self):
        """
        Tests if the IRC server maintains a connection and consistently responds
        to basic commands over a sustained period. This helps identify server
        crashes or unresponsiveness during a typical test run duration.
        """
        initial_user_nick = "StabilityProber" # Renamed for clarity within this test
        
        print(f"\n--- Running Server Stability Test ---")
        print(f"Test: Attempting to establish and maintain connection with {initial_user_nick} for stability check.")

        # Step 1: Register a client to establish a persistent connection
        try:
            # The IrcTestBase.setUp method already establishes a socket.
            # We just need to register the client on it.
            # self.register_client updates self.current_nick with the server-assigned nick
            registration_responses = self.register_client(initial_user_nick, "stableuser", "Stability User")
            self.assert_irc_reply(registration_responses, RPL_WELCOME, "Welcome") # Assert successful registration
            print(f"Test: Client {self.current_nick} (originally {initial_user_nick}) registered successfully. Ready to probe stability.")
        except Exception as e:
            self.fail(f"Failed to register client for stability test: {e}")

        # Step 2: Keep connection alive and periodically send PING/receive PONG
        # This loop will run for a total of 10 seconds, sending PINGs every 2 seconds.
        # Adjust duration (total_duration) and interval (ping_interval) as needed.
        total_duration = 10 # seconds
        ping_interval = 2   # seconds
        start_time = time.time()
        ping_count = 0

        while time.time() - start_time < total_duration:
            ping_count += 1
            ping_message_token = f"ping.probe.{ping_count}"
            print(f"Test: Sending PING command (ping #{ping_count})...")
            
            try:
                send_irc_command(self.sock, f"PING :{ping_message_token}")
                # Use a generous timeout for receiving response, allowing for server processing
                ping_pong_response = recv_irc_response(self.sock, timeout=1.5) 
                
                # Check for PONG response. The exact PONG format might vary,
                # so we will accept either the echoed token or ':localhost'.
                expected_pong_echo = f"PONG :{ping_message_token}"
                expected_pong_localhost = "PONG :localhost"

                pong_received = any(expected_pong_echo in line for line in ping_pong_response) or \
                                any(expected_pong_localhost in line for line in ping_pong_response)

                self.assertTrue(
                    pong_received,
                    f"Expected PONG response ('{expected_pong_echo}' or '{expected_pong_localhost}') not received. Full response: {ping_pong_response}"
                )
                print(f"Test: Server responded to PING with PONG. Still stable.")

            except socket.error as se:
                self.fail(f"Socket error during stability check (server likely disconnected): {se}")
            except Exception as e:
                self.fail(f"Unexpected error during stability test: {e}")
            
            # Wait for the next interval, but don't exceed total duration
            time_elapsed = time.time() - start_time
            if time_elapsed + ping_interval < total_duration:
                time.sleep(ping_interval)
            else:
                time.sleep(total_duration - time_elapsed) # Sleep for remaining time

        # Step 3: Perform one final action to confirm server is still alive
        print(f"Test: Sustained connection for {total_duration} seconds. Performing final check (NICK change).")
        final_nick = "stillalive" # Changed to lowercase
        old_nick_for_assertion = self.current_nick # Capture the current (server-assigned) nick before changing it
        
        try:
            send_irc_command(self.sock, f"NICK {final_nick}")
            nick_change_response = recv_irc_response(self.sock, timeout=1.0)
            
            # The server should respond with a NICK message from the old nick to the new nick
            # Use old_nick_for_assertion as the sender, which is the server's assigned nick for the client.
            self.assert_message_received(nick_change_response, old_nick_for_assertion, "NICK", final_nick)
            
            # Update the client's tracked nickname in the test instance after successful change
            self.current_nick = final_nick 
            print(f"Test: Server allowed NICK change to {self.current_nick}. Server appears to be fully stable.")
        except Exception as e:
            self.fail(f"Server became unresponsive or disconnected during final NICK change: {e}")

        print(f"--- Server Stability Test COMPLETED SUCCESSFULLY. ---")

if __name__ == '__main__':
    unittest.main(exit=False)
