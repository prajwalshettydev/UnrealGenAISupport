import socket
import json
import unreal

def handle_handshake(message):
    # Echo back the message as a simple response
    return {"success": True, "message": f"Received: {message}"}

def start_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('localhost', 9877))
    server_socket.listen(1)
    print("Unreal Engine socket server started on port 9877")

    while True:
        conn, addr = server_socket.accept()
        data = conn.recv(1024)
        if data:
            command = json.loads(data.decode())
            if command["type"] == "handshake":
                response = handle_handshake(command["message"])
                conn.sendall(json.dumps(response).encode())
            else:
                conn.sendall(json.dumps({"error": "Unknown command"}).encode())
        conn.close()

# Run the server in a background thread
import threading
thread = threading.Thread(target=start_server)
thread.daemon = True
thread.start()