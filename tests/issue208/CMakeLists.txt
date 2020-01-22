set (ISSUE208_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (ISSUE208_SRC ${ISSUE208_SRC_DIR}/main.cpp)

set (ISSUE208_INC_DIR ${ISSUE208_SRC_DIR}/../../)

include_directories ("${ISSUE208_SRC_DIR}")
include_directories ("${ISSUE208_INC_DIR}")

add_executable (issue208 ${ISSUE208_SRC}) 

if (WIN32)
    set (ISSUE208_LDLIBS yasio)
else ()
    set (ISSUE208_LDLIBS yasio pthread)
endif()

target_link_libraries (issue208 ${ISSUE208_LDLIBS})
