#!/usr/bin/env python3
"""
Basic Python Server for STOMP Assignment – Stage 3.3

IMPORTANT:
DO NOT CHANGE the server name or the basic protocol.
Students should EXTEND this server by implementing
the methods below.
"""

import socket
import sys
import threading


SERVER_NAME = "STOMP_PYTHON_SQL_SERVER"  # DO NOT CHANGE!
DB_FILE = "stomp_server.db"              # DO NOT CHANGE!


def recv_null_terminated(sock: socket.socket) -> str:
    data = b""
    while True:
        chunk = sock.recv(1024)
        if not chunk:
            return ""
        data += chunk
        if b"\0" in data:
            msg, _ = data.split(b"\0", 1)
            return msg.decode("utf-8", errors="replace")


def init_database():
    import sqlite3
    conn = sqlite3.connect(DB_FILE)
    cursor = conn.cursor()
    
    # Users table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            username TEXT PRIMARY KEY,
            passcode TEXT NOT NULL,
            registration_time TEXT NOT NULL
        )
    ''')
    
    # Login history table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS login_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            login_time TEXT NOT NULL,
            logout_time TEXT,
            FOREIGN KEY (username) REFERENCES users(username)
        )
    ''')
    
    # Files table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS uploaded_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            filename TEXT NOT NULL,
            game_channel TEXT,
            upload_time TEXT NOT NULL,
            FOREIGN KEY (username) REFERENCES users(username)
        )
    ''')
    
    conn.commit()
    conn.close()
    print(f"[{SERVER_NAME}] Database initialized")


def execute_sql_command(sql_command: str) -> str:
    import sqlite3
    import json
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute(sql_command)
        conn.commit()
        rows_affected = cursor.rowcount
        conn.close()
        return json.dumps({"status": "success", "rows_affected": rows_affected})
    except Exception as e:
        return json.dumps({"status": "error", "message": str(e)})


def execute_sql_query(sql_query: str) -> str:
    import sqlite3
    import json
    try:
        conn = sqlite3.connect(DB_FILE)
        cursor = conn.cursor()
        cursor.execute(sql_query)
        columns = [description[0] for description in cursor.description] if cursor.description else []
        results = cursor.fetchall()
        conn.close()
        return json.dumps({"status": "success", "columns": columns, "rows": results})
    except Exception as e:
        return json.dumps({"status": "error", "message": str(e)})


def handle_client(client_socket: socket.socket, addr):
    print(f"[{SERVER_NAME}] Client connected from {addr}")

    try:
        while True:
            message = recv_null_terminated(client_socket)
            if message == "":
                break

            print(f"[{SERVER_NAME}] Received:")
            print(message)
            
            # Determine if it's a query or command
            sql_upper = message.strip().upper()
            if sql_upper.startswith('SELECT'):
                response = execute_sql_query(message)
            else:
                response = execute_sql_command(message)
            
            client_socket.sendall((response + "\0").encode('utf-8'))

    except Exception as e:
        print(f"[{SERVER_NAME}] Error handling client {addr}: {e}")
    finally:
        try:
            client_socket.close()
        except Exception:
            pass
        print(f"[{SERVER_NAME}] Client {addr} disconnected")


def start_server(host="127.0.0.1", port=7778):
    # Initialize database when server starts
    init_database()
    
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((host, port))
        server_socket.listen(5)
        print(f"[{SERVER_NAME}] Server started on {host}:{port}")
        print(f"[{SERVER_NAME}] Waiting for connections...")

        while True:
            client_socket, addr = server_socket.accept()
            t = threading.Thread(
                target=handle_client,
                args=(client_socket, addr),
                daemon=True
            )
            t.start()

    except KeyboardInterrupt:
        print(f"\n[{SERVER_NAME}] Shutting down server...")
    finally:
        try:
            server_socket.close()
        except Exception:
            pass


if __name__ == "__main__":
    port = 7778
    if len(sys.argv) > 1:
        raw_port = sys.argv[1].strip()
        try:
            port = int(raw_port)
        except ValueError:
            print(f"Invalid port '{raw_port}', falling back to default {port}")

    start_server(port=port)
