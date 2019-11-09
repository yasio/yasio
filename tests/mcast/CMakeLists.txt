set (MCASTTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (MCASTTEST_SRC ${MCASTTEST_SRC_DIR}/main.cpp)

set (MCASTTEST_INC_DIR ${MCASTTEST_SRC_DIR}/../../)

include_directories ("${MCASTTEST_SRC_DIR}")
include_directories ("${MCASTTEST_INC_DIR}")

add_executable (mcasttest ${MCASTTEST_SRC}) 

if (WIN32)
    set (MCASTTEST_LDLIBS yasio)
else ()
    set (MCASTTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (mcasttest ${MCASTTEST_LDLIBS})
