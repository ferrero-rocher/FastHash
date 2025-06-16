# C++ Key-Value Store

A high-performance, thread-safe key-value store implementation in C++ with support for TTL, persistence, and concurrent client handling.

## Features

- **In-Memory Storage**: Fast key-value operations with O(1) complexity
- **TTL Support**: Automatic expiration of keys with configurable time-to-live
- **Persistence**: Save/load operations for data durability
- **Concurrent Access**: Thread-safe operations with mutex protection
- **Statistics**: Real-time monitoring of store operations and performance
- **Logging**: Comprehensive logging system with file and console output
- **Client-Server Architecture**: TCP/IP based client-server model
- **Clean Architecture**: Modular design with clear separation of concerns

## Repository Structure

```
├── include/                # Header files
│   ├── CommandHandler.h    # Command processing
│   ├── KeyValueStore.h     # Core store implementation
│   ├── Logger.h           # Logging system
│   └── Server.h           # Server implementation
├── src/                   # Source files
│   ├── CommandHandler.cpp
│   ├── KeyValueStore.cpp
│   ├── Logger.cpp
│   ├── Server.cpp
│   ├── client.cpp        # Client implementation
│   └── main.cpp          # Server entry point
├── ARCHITECTURE.md       # System architecture documentation
├── CMakeLists.txt        # CMake build configuration
└── README.md             # This documentation
```

## Building the Project

### Prerequisites
- C++17 or later
- CMake 3.10 or later
- Windows (for Windows-specific socket implementation)

### Build Steps

```bash
# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build .
```

## Usage

### Starting the Server

```bash
# Default port (8080)
./kvstore_server

# Custom port
./kvstore_server 9090
```

### Using the Client

```bash
# Connect to server
./kvstore_client localhost 8080

# Example session:
> SET key1 value1
OK
> GET key1
value1
> SET key2 value2 60  # Set with 60 second TTL
OK
> DEL key1
OK
```

## Supported Commands

### Basic Operations
- `SET <key> <value> [ttl]` - Store a value with optional TTL
- `GET <key>` - Retrieve a value
- `DEL <key>` - Delete a key
- `EXISTS <key>` - Check if key exists
- `EXPIRE <key> <seconds>` - Set TTL for existing key
- `TTL <key>` - Get remaining TTL for key
- `KEYS` - List all keys
- `CLEAR` - Remove all keys
- `FLUSH` - Clear all data and statistics

### Persistence
- `SAVE <filename>` - Save current state to file
- `LOAD <filename>` - Load state from file

### Statistics
- `STATS` - Display store statistics

## Architecture

The system follows a clean architecture with the following components:

1. **Server**: Handles TCP/IP connections and client communication
2. **CommandHandler**: Processes and validates client commands
3. **KeyValueStore**: Core data structure with thread-safe operations
4. **Logger**: Handles system logging and monitoring

For detailed architecture information, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Implementation Details

### Concurrency
- Thread-safe operations using mutex protection
- Background TTL cleaner thread
- Concurrent client handling

### Data Structures
- `unordered_map` for O(1) key-value operations
- Thread-safe wrapper for concurrent access
- TTL tracking with chrono timestamps

### Persistence
- Text-based file format for data storage
- Manual save/load operations via SAVE and LOAD commands
- File-based data durability

### Logging
- Thread-safe logging operations
- Support for both console and file output
- Timestamp and log level tracking

## Testing

The project includes comprehensive tests for all major components:

```bash
# Run all tests
cd build
ctest
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details. 