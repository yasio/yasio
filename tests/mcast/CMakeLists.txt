set (target_name mcasttest)

set (MCASTTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (MCASTTEST_INC_DIR ${MCASTTEST_SRC_DIR}/../../)

set (MCASTTEST_SRC ${MCASTTEST_SRC_DIR}/main.cpp)

include_directories ("${MCASTTEST_SRC_DIR}")
include_directories ("${MCASTTEST_INC_DIR}")

add_executable (${target_name} ${MCASTTEST_SRC}) 

if (WIN32)
    set (MCASTTEST_LDLIBS yasio)
else ()
    set (MCASTTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${MCASTTEST_LDLIBS})

ConfigTargetSSL(${target_name})
