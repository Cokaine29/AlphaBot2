import socket
import sys

# Change this to your board's IP if it changes!
IP = "10.49.181.159"
PORT = 23

print(f"Connecting to {IP}:{PORT}...")
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((IP, PORT))
    print("Connected successfully! Listening for logs (Press Ctrl+C to exit)...\n")
    s.sendall(b"\n") # Send a dummy newline to ensure TelnetStream registers the connection
    while True:
        data = s.recv(1024)
        if not data:
            print("\nConnection closed by host.")
            break
        sys.stdout.write(data.decode("utf-8", errors="ignore"))
        sys.stdout.flush()
except KeyboardInterrupt:
    print("\nDisconnecting...")
except Exception as e:
    print(f"\nError: {e}")
finally:
    s.close()
