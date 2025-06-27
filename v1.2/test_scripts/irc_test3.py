import socket
import threading
import time
import traceback
import sys

HOST = '127.0.0.1'
PORT = 6667
PASSWORD = "testpass"
CHANNEL = "#bravery"
TIMEOUT = 10

class IRCClient:
    def __init__(self, nick):
        self.nick = nick
        self.user = f"user_{nick}"
        self.realname = f"Real {nick}"
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.responses = []
        self.lock = threading.Lock()

    def connect_and_register(self, send_user=True, send_pass=True):
        try:
            self.sock.connect((HOST, PORT))
            self.send(f"CAP LS 302")
            time.sleep(0.1)
            if send_pass:
                self.send(f"PASS {PASSWORD}")
                time.sleep(0.1)
            self.send(f"NICK {self.nick}")
            time.sleep(0.1)
            if send_user:
                self.send(f"USER {self.user} 0 * :{self.realname}")
            self.wait_for_welcome()
        except Exception as e:
            lineno = sys.exc_info()[2].tb_lineno
            print(f"[{self.nick}] ERROR during registration (line {lineno}): {e}")
            raise

    def wait_for_welcome(self):
        deadline = time.time() + TIMEOUT
        while time.time() < deadline:
            resp = self.receive()
            if resp:
                self.responses.extend(resp)
                if any(" 004 " in line for line in resp):
                    print(f"[{self.nick}] Registration complete.")
                    return
        raise TimeoutError(f"[{self.nick}] Timed out waiting for 004 welcome.")

    def send(self, msg):
        print(f"[{self.nick}] → {msg}")
        try:
            self.sock.sendall((msg + "\r\n").encode())
        except Exception as e:
            lineno = sys.exc_info()[2].tb_lineno
            print(f"[{self.nick}] ERROR send (line {lineno}): {e}")

    def receive(self):
        try:
            self.sock.settimeout(0.5)
            data = self.sock.recv(4096)
            lines = data.decode(errors='ignore').split("\r\n")
            for line in lines:
                if line:
                    print(f"[{self.nick}] Received: {line}")
            return [line for line in lines if line]
        except:
            return []

    def close(self):
        self.sock.close()

def run_full_test():
    try:
        c1 = IRCClient("Brave1")
        c2 = IRCClient("Brave2")
        c3 = IRCClient("Brave3")

        c1.connect_and_register()
        c2.connect_and_register()
        c3.connect_and_register()

        c1.send(f"JOIN {CHANNEL}")
        c2.send(f"JOIN {CHANNEL}")
        c3.send(f"JOIN {CHANNEL}")
        time.sleep(0.5)

        # Grant ops, kick, invite, mode i
        c1.send(f"MODE {CHANNEL} +o {c1.nick}")
        time.sleep(0.2)
        c1.send(f"KICK {CHANNEL} {c2.nick} :Get out")
        c2.send(f"JOIN {CHANNEL}")  # should fail
        c1.send(f"INVITE {c2.nick} {CHANNEL}")
        c2.send(f"JOIN {CHANNEL}")  # should succeed
        c1.send(f"MODE {CHANNEL} +i")
        c3.send(f"PART {CHANNEL}")
        c3.send(f"JOIN {CHANNEL}")  # should fail unless invited
        c1.send(f"INVITE {c3.nick} {CHANNEL}")
        c3.send(f"JOIN {CHANNEL}")
        time.sleep(0.5)

        # Invalid MODE flag
        c1.send(f"MODE {CHANNEL} +z")
        time.sleep(0.2)

        # Missing params
        c2.send("MODE")
        c2.send("KICK")
        c2.send("JOIN")
        time.sleep(0.2)

        # Non-ops trying to set topic
        c3.send(f"TOPIC {CHANNEL} :Forbidden topic")
        time.sleep(0.2)

        # Ghost kick
        c1.send(f"KICK {CHANNEL} GhostUser")
        time.sleep(0.2)

        # Channel key enforcement
        c1.send(f"MODE {CHANNEL} +k watermelon")
        c3.send(f"PART {CHANNEL}")
        c3.send(f"JOIN {CHANNEL}")  # no key
        c3.send(f"JOIN {CHANNEL} wrongkey")  # bad key
        c3.send(f"JOIN {CHANNEL} watermelon")  # correct
        time.sleep(0.5)

        # Nickname collision
        c4 = IRCClient("Brave1")  # same nick as c1
        try:
            c4.connect_and_register()
        except Exception as e:
            print(f"[Brave1-Collision] Expected nick conflict: {e}")
        finally:
            c4.close()

        # Incomplete registration: no PASS
        c5 = IRCClient("NoPass")
        try:
            c5.connect_and_register(send_pass=False)
        except:
            print("[NoPass] Expected registration failure.")
        finally:
            c5.close()
        # Set a user limit of 2
        c1.send(f"MODE {CHANNEL} +l 2")
        time.sleep(0.2)
        
        # Disconnect and reconnect c3 (to test limit enforcement on join)
        c3.send(f"PART {CHANNEL} :making room")
        time.sleep(0.2)
        c3.send(f"JOIN {CHANNEL}")  # Should work—2 users now (c1 + c2)
        
        # Try adding a 4th client
        c4 = IRCClient("Brave4")
        c4.connect_and_register()
        c4.send(f"JOIN {CHANNEL}")  # Should get 471 ERR_CHANNELISFULL
        time.sleep(0.5)
        
        # Now lift the limit and try again
        c1.send(f"MODE {CHANNEL} -l")
        time.sleep(0.2)
        c4.send(f"JOIN {CHANNEL}")  # Should succeed now
        time.sleep(0.5)
        
        # Flooder: repeat garbage commands fast
        for _ in range(10):
            c1.send("JUNKCOMMAND")
            c1.send("MODE")
            c1.send("PART")  # missing channel
        time.sleep(0.5)

        print("\n✅ All extended test cases executed.")
    except Exception as e:
        lineno = sys.exc_info()[2].tb_lineno
        print(f"[FATAL] Exception at line {lineno}: {traceback.format_exc()}")
    finally:
        for client in locals().values():
            if isinstance(client, IRCClient):
                client.close()

if __name__ == "__main__":
    run_full_test()
