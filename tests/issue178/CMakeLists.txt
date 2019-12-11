set (ISSUE178_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (ISSUE178_SRC ${ISSUE178_SRC_DIR}/main.cpp)

set (ISSUE178_INC_DIR ${ISSUE178_SRC_DIR}/../../)

include_directories ("${ISSUE178_SRC_DIR}")
include_directories ("${ISSUE178_INC_DIR}")

add_executable (issue178 ${ISSUE178_SRC}) 

if (WIN32)
    set (ISSUE178_LDLIBS yasio)
else ()
    set (ISSUE178_LDLIBS yasio pthread)
endif()

target_link_libraries (issue178 ${ISSUE178_LDLIBS})
