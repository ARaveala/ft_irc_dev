import socket
import threading
import time

SERVER = 'localhost'
PORT = 6668
CHANNEL = '#loadtest'
MAX_CLIENTS = 110
JOIN_DELAY = 0.05  # Seconds between joins to avoid burst

def connect_client(client_id):
    try:
        nick = f'testuser{client_id}'
        user = f'user{client_id}'
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((SERVER, PORT))
        s.sendall(f"PASS Hell1\r\n".encode())
        s.sendall(f"NICK {nick}\r\n".encode())
        s.sendall(f"USER {user} 0 * :Test User {client_id}\r\n".encode())
        time.sleep(0.5)
        s.sendall(f"JOIN {CHANNEL}\r\n".encode())
        print(f"[+] Client {nick} joined {CHANNEL}")
        # Keep socket open to simulate a real client
        while True:
            time.sleep(10)
    except Exception as e:
        print(f"[!] Client {client_id} error: {e}")

for i in range(MAX_CLIENTS):
    threading.Thread(target=connect_client, args=(i,), daemon=True).start()
    time.sleep(JOIN_DELAY)

print(f"🚀 Launched up to {MAX_CLIENTS} simulated clients.")
while True:
    time.sleep(60)
