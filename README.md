# Key-Value Store Server

A modular C++17 TCP key-value store server.

## Components
- **Server**: TCP server listening on port 8080
- **ThreadPool**: For handling concurrent requests
- **KeyValueStore**: In-memory key-value storage
- **CommandHandler**: Parses and executes commands

## Build

```
mkdir build
cd build
cmake ..
cmake --build .
```

## Run

```
./kvstore_server
```

## Structure
- `src/`: Source files
- `include/`: Header files 