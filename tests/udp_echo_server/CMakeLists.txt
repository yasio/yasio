set (UDP_ECHO_SERVER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (UDP_ECHO_SERVER_SRC ${UDP_ECHO_SERVER_SRC_DIR}/main.cpp)

set (UDP_ECHO_SERVER_INC_DIR ${UDP_ECHO_SERVER_SRC_DIR}/../../)

include_directories ("${UDP_ECHO_SERVER_SRC_DIR}")
include_directories ("${UDP_ECHO_SERVER_INC_DIR}")

add_executable (udp_echo_server ${UDP_ECHO_SERVER_SRC}) 

if (WIN32)
    set (UDP_ECHO_SERVER_LDLIBS yasio)
else ()
    set (UDP_ECHO_SERVER_LDLIBS yasio pthread)
endif()

target_link_libraries (udp_echo_server ${UDP_ECHO_SERVER_LDLIBS})
