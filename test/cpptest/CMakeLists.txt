set (CPPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (CPPTEST_SRC ${CPPTEST_SRC_DIR}/cpptest.cpp)

set (CPPTEST_INC_DIR ${CPPTEST_SRC_DIR}/../../)

include_directories ("${CPPTEST_SRC_DIR}")
include_directories ("${CPPTEST_INC_DIR}")

add_executable (cpptest ${CPPTEST_SRC}) 

if (WIN32)
    set (CPPTEST_LDLIBS yasio)
else ()
    set (CPPTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (cpptest ${CPPTEST_LDLIBS})
