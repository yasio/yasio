set (target_name speedtest)
set (SPEEDTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (SPEEDTEST_SRC 
    ${SPEEDTEST_SRC_DIR}/main.cpp
)

set (SPEEDTEST_INC_DIR ${SPEEDTEST_SRC_DIR}/../../)

include_directories ("${SPEEDTEST_SRC_DIR}")
include_directories ("${SPEEDTEST_INC_DIR}")

add_executable (${target_name} ${SPEEDTEST_SRC}) 

yasio_config_app_depends(${target_name})
