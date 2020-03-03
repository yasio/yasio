set (target_name issue166)

set (ISSUE166_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE166_INC_DIR ${ISSUE166_SRC_DIR}/../../)

set (ISSUE166_SRC ${ISSUE166_SRC_DIR}/main.cpp)

include_directories ("${ISSUE166_SRC_DIR}")
include_directories ("${ISSUE166_INC_DIR}")

add_executable (${target_name} ${ISSUE166_SRC}) 

if (WIN32)
    set (ISSUE166_LDLIBS yasio)
else ()
    set (ISSUE166_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${ISSUE166_LDLIBS})

ConfigTargetSSL(${target_name})
