cmake_minimum_required(VERSION 3.10)
project(kvstore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SERVER_SOURCES
    main.cpp
    Server.cpp
    KeyValueStore.cpp
    CommandHandler.cpp
    Logger.cpp
)

set(CLIENT_SOURCES
    client.cpp
    Logger.cpp
)

# Add test files
set(TEST_SOURCES
    test_kvstore.cpp
)

# Create main executable
add_executable(kvstore_server ${SERVER_SOURCES})

# Create client executable
add_executable(kvstore_client ${CLIENT_SOURCES})

# Create test executables
add_executable(test_kvstore test_kvstore.cpp KeyValueStore.cpp CommandHandler.cpp Logger.cpp)

# Include directories
target_include_directories(kvstore_server PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(kvstore_client PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(test_kvstore PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link libraries
target_link_libraries(kvstore_server ws2_32)
target_link_libraries(kvstore_client ws2_32)
target_link_libraries(test_kvstore ws2_32)

# Enable testing
enable_testing()
add_test(NAME test_kvstore COMMAND test_kvstore) 