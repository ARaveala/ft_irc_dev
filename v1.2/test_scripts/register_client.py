# register_client.py
import socket
import time

def register_client(nick, username, realname, channel=None, host='localhost', port=6667):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))

    def send(msg):
        full_msg = msg + "\r\n"
        print(f"[{nick}] → {msg}")
        sock.sendall(full_msg.encode())
        time.sleep(0.1)

    send("PASS testpass")
    send(f"NICK {nick}")
    send(f"USER {username} 0 * :{realname}")
    if channel:
        send(f"JOIN {channel}")

    return sock  # Keep socket open for further use
