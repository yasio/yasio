set(target_name echo_client)

set (ECHO_CLIENT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ECHO_CLIENT_INC_DIR ${ECHO_CLIENT_SRC_DIR}/../../)

set (ECHO_CLIENT_SRC ${ECHO_CLIENT_SRC_DIR}/main.cpp)


include_directories ("${ECHO_CLIENT_SRC_DIR}")
include_directories ("${ECHO_CLIENT_INC_DIR}")

add_executable (${target_name} ${ECHO_CLIENT_SRC}) 

if (WIN32)
    set (ECHO_CLIENT_LDLIBS yasio)
else ()
    set (ECHO_CLIENT_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ECHO_CLIENT_LDLIBS})

ConfigTargetDepends(${target_name})
