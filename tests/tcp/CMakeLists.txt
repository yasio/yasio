set(target_name tcptest)

set (TCPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (TCPTEST_INC_DIR ${TCPTEST_SRC_DIR}/../../)

set (TCPTEST_SRC ${TCPTEST_SRC_DIR}/main.cpp)


include_directories ("${TCPTEST_SRC_DIR}")
include_directories ("${TCPTEST_INC_DIR}")

add_executable (${target_name} ${TCPTEST_SRC}) 

if (WIN32)
    set (TCPTEST_LDLIBS yasio)
else ()
    set (TCPTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${TCPTEST_LDLIBS})

ConfigTargetSSL(${target_name})
