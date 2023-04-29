cmake_minimum_required(VERSION 3.12)
project(multiple_chat_room)

add_executable(client
    ${PROJECT_SOURCE_DIR}/chatroom.h
    ${PROJECT_SOURCE_DIR}/client.c
)
target_link_libraries(client PRIVATE "-lpthread")

add_executable(server
    ${PROJECT_SOURCE_DIR}/chatroom.h
    ${PROJECT_SOURCE_DIR}/server.c
)
target_link_libraries(server PRIVATE "-lm")
