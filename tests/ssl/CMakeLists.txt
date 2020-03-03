set(target_name ssltest)

set (SSLTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (SSLTEST_INC_DIR ${SSLTEST_SRC_DIR}/../../)

set (SSLTEST_SRC ${SSLTEST_SRC_DIR}/main.cpp)

include_directories ("${SSLTEST_SRC_DIR}")
include_directories ("${SSLTEST_INC_DIR}")

add_executable (${target_name} ${SSLTEST_SRC}) 

if (WIN32)
    set (SSLTEST_LDLIBS yasio)
else ()
    set (SSLTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${SSLTEST_LDLIBS})

# link ssl stubs
ConfigTargetSSL(${target_name})
