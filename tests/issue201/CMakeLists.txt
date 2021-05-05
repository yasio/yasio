set (target_name issue201)

set (ISSUE201_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE201_INC_DIR ${ISSUE201_SRC_DIR}/../../)

set (ISSUE201_SRC ${ISSUE201_SRC_DIR}/main.cpp)

include_directories ("${ISSUE201_SRC_DIR}")
include_directories ("${ISSUE201_INC_DIR}")

add_executable (${target_name} ${ISSUE201_SRC}) 

if (WIN32)
    set (ISSUE201_LDLIBS yasio)
else ()
    set (ISSUE201_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ISSUE201_LDLIBS})

ConfigTargetDepends(${target_name})
