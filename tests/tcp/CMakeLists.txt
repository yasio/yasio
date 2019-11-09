set (TCPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (TCPTEST_SRC ${TCPTEST_SRC_DIR}/main.cpp)

set (TCPTEST_INC_DIR ${TCPTEST_SRC_DIR}/../../)

include_directories ("${TCPTEST_SRC_DIR}")
include_directories ("${TCPTEST_INC_DIR}")

add_executable (tcptest ${TCPTEST_SRC}) 

if (WIN32)
    set (TCPTEST_LDLIBS yasio)
else ()
    set (TCPTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (tcptest ${TCPTEST_LDLIBS})
