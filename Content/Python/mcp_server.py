import socket
import json
import sys
from mcp.server.fastmcp import FastMCP

# THIS FILE WILL RUN OUTSIDE THE UNREAL ENGINE SCOPE, 
# DO NOT IMPORT UNREAL MODULES HERE OR EXECUTE IT IN THE UNREAL ENGINE PYTHON INTERPRETER

# Create an MCP server
mcp = FastMCP("UnrealHandshake")

# Function to send a message to Unreal Engine via socket
def send_to_unreal(message: str):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('localhost', 9877))  # Unreal listens on port 9877
        command = json.dumps({"type": "handshake", "message": message})
        s.sendall(command.encode())
        response = s.recv(1024)
        return json.loads(response.decode())

# Define a tool for Claude to call
@mcp.tool()
def handshake_test(message: str) -> str:
    """Send a handshake message to Unreal Engine"""
    try:
        response = send_to_unreal(message)
        if response.get("success"):
            return f"Handshake successful: {response['message']}"
        else:
            return f"Handshake failed: {response.get('error', 'Unknown error')}"
    except Exception as e:
        return f"Error communicating with Unreal: {str(e)}"


if __name__ == "__main__":
    try:
        print("Server starting...", file=sys.stderr)
        mcp.run()
    except Exception as e:
        print(f"Server crashed with error: {e}", file=sys.stderr)
        traceback.print_exc(file=sys.stderr)
        raise