set (SSLTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (SSLTEST_SRC ${SSLTEST_SRC_DIR}/main.cpp)

set (SSLTEST_INC_DIR ${SSLTEST_SRC_DIR}/../../)

include_directories ("${SSLTEST_SRC_DIR}")
include_directories ("${SSLTEST_INC_DIR}")

add_executable (ssltest ${SSLTEST_SRC}) 

if (WIN32)
    set (SSLTEST_LDLIBS yasio)
else ()
    set (SSLTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (ssltest ${SSLTEST_LDLIBS})
