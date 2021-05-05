set (target_name issue256)

set (ISSUE256_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE256_INC_DIR ${ISSUE256_SRC_DIR}/../../)

set (ISSUE256_SRC ${ISSUE256_SRC_DIR}/main.cpp)

include_directories ("${ISSUE256_SRC_DIR}")
include_directories ("${ISSUE256_INC_DIR}")

add_executable (${target_name} ${ISSUE256_SRC}) 

if (WIN32)
    set (ISSUE256_LDLIBS yasio)
else ()
    set (ISSUE256_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ISSUE256_LDLIBS})

ConfigTargetDepends(${target_name})
