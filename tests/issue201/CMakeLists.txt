set (ISSUE201_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (ISSUE201_SRC ${ISSUE201_SRC_DIR}/main.cpp)

set (ISSUE201_INC_DIR ${ISSUE201_SRC_DIR}/../../)

include_directories ("${ISSUE201_SRC_DIR}")
include_directories ("${ISSUE201_INC_DIR}")

add_executable (issue201 ${ISSUE201_SRC}) 

if (WIN32)
    set (ISSUE201_LDLIBS yasio)
else ()
    set (ISSUE201_LDLIBS yasio pthread)
endif()

target_link_libraries (issue201 ${ISSUE201_LDLIBS})
