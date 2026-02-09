import socket
import time
import threading

HOST = '127.0.0.1'
PORT = 5000

def run_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind((HOST, PORT))
        server_socket.listen(1)
        print(f"Mock Server listening on {HOST}:{PORT}")
        
        conn, addr = server_socket.accept()
        print(f"Connected by {addr}")
        
        # Start a thread to send data periodically
        def send_data():
            commands = ["cmd:next\n", "VR: 1234\n", "cmd:pause\n", "VR: 100\n", "cmd:play\n", "cmd:previous\n", "VR: 4095\n"]
            for cmd in commands:
                time.sleep(1)
                if not conn: break
                try:
                    # Simulate fragmentation
                    for char in cmd:
                        conn.sendall(char.encode('utf-8'))
                        time.sleep(0.05) # Small delay between chars
                    print(f"Sent: {cmd.strip()}")
                except Exception as e:
                    print(f"Send error: {e}")
                    break
        
        sender = threading.Thread(target=send_data)
        sender.start()
        
        # Main loop to receive data
        while True:
            data = conn.recv(1024)
            if not data:
                break
            print(f"Received: {data.decode('utf-8')}")
            
    except Exception as e:
        print(f"Server error: {e}")
    finally:
        server_socket.close()

if __name__ == '__main__':
    run_server()
