set (target_name speedtest)
set (SPEEDTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (SPEEDTEST_SRC 
    ${SPEEDTEST_SRC_DIR}/main.cpp
)

set (SPEEDTEST_INC_DIR ${SPEEDTEST_SRC_DIR}/../../)

include_directories ("${SPEEDTEST_SRC_DIR}")
include_directories ("${SPEEDTEST_INC_DIR}")

add_executable (${target_name} ${SPEEDTEST_SRC}) 

if (WIN32)
    set (SPEEDTEST_LDLIBS yasio)
else ()
    set (SPEEDTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${SPEEDTEST_LDLIBS})

ConfigTargetDepends(${target_name})
