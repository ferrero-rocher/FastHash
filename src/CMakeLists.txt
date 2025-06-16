add_executable(kvstore_server
    main.cpp
    Server.cpp
    KeyValueStore.cpp
    CommandHandler.cpp
    Logger.cpp
)

add_executable(test_threadpool
    test_threadpool.cpp
)

add_executable(test_kvstore
    test_kvstore.cpp
    KeyValueStore.cpp
    CommandHandler.cpp
    Logger.cpp
)

add_executable(test_client
    test_client.cpp
)

add_executable(kvstore_client client.cpp)

target_include_directories(kvstore_server PRIVATE ../include)
target_include_directories(test_threadpool PRIVATE ../include)
target_include_directories(test_kvstore PRIVATE ../include)
target_include_directories(test_client PRIVATE ../include)
target_include_directories(kvstore_client PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    target_link_libraries(kvstore_server ws2_32)
    target_link_libraries(test_client ws2_32)
    target_link_libraries(kvstore_client ws2_32)
    target_link_libraries(test_kvstore ws2_32)
endif() 