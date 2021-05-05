set (target_name issue245)

set (ISSUE245_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE245_INC_DIR ${ISSUE245_SRC_DIR}/../../)

set (ISSUE245_SRC ${ISSUE245_SRC_DIR}/main.cpp)

include_directories ("${ISSUE245_SRC_DIR}")
include_directories ("${ISSUE245_INC_DIR}")

add_executable (${target_name} ${ISSUE245_SRC}) 

if (WIN32)
    set (ISSUE245_LDLIBS yasio)
else ()
    set (ISSUE245_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ISSUE245_LDLIBS})

ConfigTargetDepends(${target_name})
