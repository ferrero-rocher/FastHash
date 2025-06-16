import socket
import time
import os
import sys
import json
from datetime import datetime

def send_command(sock, command):
    sock.sendall(command.encode() + b'\n')
    response = sock.recv(1024).decode().strip()
    return response

def test_connection():
    print("\n=== Testing Basic Connection ===")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('localhost', 8080))
    print("✓ Connected to server")
    return sock

def test_basic_operations(sock):
    print("\n=== Testing Basic Operations ===")
    
    # Test SET and GET
    print("\nTesting SET and GET:")
    assert send_command(sock, "SET test_key test_value") == "OK"
    assert send_command(sock, "GET test_key") == "test_value"
    print("✓ SET and GET working")
    
    # Test DEL
    print("\nTesting DEL:")
    assert send_command(sock, "DEL test_key") == "1"
    assert send_command(sock, "GET test_key") == "Key not found"
    print("✓ DEL working")
    
    # Test KEYS
    print("\nTesting KEYS:")
    send_command(sock, "SET key1 value1")
    send_command(sock, "SET key2 value2")
    keys = send_command(sock, "KEYS")
    assert "key1" in keys and "key2" in keys
    print("✓ KEYS working")

def test_ttl(sock):
    print("\n=== Testing TTL ===")
    
    # Test SET with TTL
    print("\nTesting SET with TTL:")
    assert send_command(sock, "SET ttl_key ttl_value 2") == "OK"
    assert send_command(sock, "GET ttl_key") == "ttl_value"
    print("✓ Key set with TTL")
    
    # Wait for TTL to expire
    print("Waiting for TTL to expire...")
    time.sleep(3)
    assert send_command(sock, "GET ttl_key") == "Key not found"
    print("✓ TTL expiration working")

def test_persistence(sock):
    print("\n=== Testing Persistence ===")
    
    # Test SAVE
    print("\nTesting SAVE:")
    send_command(sock, "SET persist_key persist_value")
    assert send_command(sock, "SAVE") == "OK"
    print("✓ SAVE working")
    
    # Test CLEAR and LOAD
    print("\nTesting CLEAR and LOAD:")
    assert send_command(sock, "CLEAR") == "OK"
    assert send_command(sock, "GET persist_key") == "Key not found"
    assert send_command(sock, "LOAD") == "OK"
    assert send_command(sock, "GET persist_key") == "persist_value"
    print("✓ CLEAR and LOAD working")
    
    # Test FLUSH
    print("\nTesting FLUSH:")
    assert send_command(sock, "FLUSH") == "OK"
    assert send_command(sock, "GET persist_key") == "Key not found"
    assert send_command(sock, "LOAD") == "OK"
    assert send_command(sock, "GET persist_key") == "persist_value"
    print("✓ FLUSH working")

def test_stats(sock):
    print("\n=== Testing Stats ===")
    stats = send_command(sock, "STATS")
    print(f"Server stats: {stats}")
    print("✓ STATS working")

def check_logs():
    print("\n=== Checking Log Files ===")
    log_dir = "logs"
    if os.path.exists(log_dir):
        log_files = [f for f in os.listdir(log_dir) if f.endswith('.log')]
        if log_files:
            latest_log = max(log_files, key=lambda x: os.path.getctime(os.path.join(log_dir, x)))
            print(f"Latest log file: {latest_log}")
            with open(os.path.join(log_dir, latest_log), 'r') as f:
                print("\nLast 5 log entries:")
                for line in f.readlines()[-5:]:
                    print(line.strip())
        else:
            print("No log files found")
    else:
        print("Logs directory not found")

def main():
    try:
        print("Starting key-value store server tests...")
        sock = test_connection()
        
        test_basic_operations(sock)
        test_ttl(sock)
        test_persistence(sock)
        test_stats(sock)
        
        sock.close()
        print("\n=== All tests completed successfully! ===")
        
        check_logs()
        
    except AssertionError as e:
        print(f"\n❌ Test failed: {str(e)}")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main() 