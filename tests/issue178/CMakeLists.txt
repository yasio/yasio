set (target_name issue178)

set (ISSUE178_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE178_INC_DIR ${ISSUE178_SRC_DIR}/../../)

set (ISSUE178_SRC ${ISSUE178_SRC_DIR}/main.cpp)

include_directories ("${ISSUE178_SRC_DIR}")
include_directories ("${ISSUE178_INC_DIR}")

add_executable (${target_name} ${ISSUE178_SRC}) 

if (WIN32)
    set (ISSUE178_LDLIBS yasio)
else ()
    set (ISSUE178_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ISSUE178_LDLIBS})

ConfigTargetSSL(${target_name})
