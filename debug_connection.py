import socket
import sys
import time

HOST = '172.24.176.1'
PORT = 5000

print(f"Attempting to connect to {HOST}:{PORT}...")

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(5)
    s.connect((HOST, PORT))
    print(f"Connected to {HOST}:{PORT}!")
    print("Waiting for data... (Press Ctrl+C to stop)")
    
    s.settimeout(None) # Remove timeout for reading
    while True:
        data = s.recv(1024)
        if not data:
            print("Connection closed by server.")
            break
        print(f"Received: {data}")
        try:
             print(f"Decoded: {data.decode('utf-8', errors='replace')}")
        except:
             pass

except socket.timeout:
    print(f"Connection timed out. Is the server running at {HOST}:{PORT}?")
except ConnectionRefusedError:
    print(f"Connection refused. Verify IP/Port and that the bridge is running.")
except KeyboardInterrupt:
    print("\nuser stopped.")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 's' in locals():
        s.close()
