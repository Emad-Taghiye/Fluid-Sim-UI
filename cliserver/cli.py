# socket_server.py (MacBook)
import socket

s = socket.socket()
s.bind(('localhost', 6300))
s.listen(1)
print("Waiting for a connection...")
conn, addr = s.accept()
print(f"Connected by {addr}")
while True:
    data = conn.recv(1024)
    if not data:
        break
    print("Received:", data)
    conn.sendall(b'ACK')

