set (target_name perftest)
set (PERFTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (PERFTEST_SRC 
    ${PERFTEST_SRC_DIR}/main.cpp
)

set (PERFTEST_INC_DIR ${PERFTEST_SRC_DIR}/../../)

include_directories ("${PERFTEST_SRC_DIR}")
include_directories ("${PERFTEST_INC_DIR}")

add_executable (${target_name} ${PERFTEST_SRC}) 

yasio_config_app_depends(${target_name})
