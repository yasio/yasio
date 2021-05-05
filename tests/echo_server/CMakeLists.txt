set(target_name echo_server)

set (ECHO_SERVER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ECHO_SERVER_INC_DIR ${ECHO_SERVER_SRC_DIR}/../../)

set (ECHO_SERVER_SRC ${ECHO_SERVER_SRC_DIR}/main.cpp)

include_directories ("${ECHO_SERVER_SRC_DIR}")
include_directories ("${ECHO_SERVER_INC_DIR}")

add_executable (${target_name} ${ECHO_SERVER_SRC}) 

if (WIN32)
    set (ECHO_SERVER_LDLIBS yasio)
else ()
    set (ECHO_SERVER_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ECHO_SERVER_LDLIBS})

ConfigTargetDepends(${target_name})
