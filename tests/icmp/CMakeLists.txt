set(target_name icmptest)

set (ICMPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ICMPTEST_INC_DIR ${ICMPTEST_SRC_DIR}/../../)

set (ICMPTEST_SRC ${ICMPTEST_SRC_DIR}/main.cpp)


include_directories ("${ICMPTEST_SRC_DIR}")
include_directories ("${ICMPTEST_INC_DIR}")

add_executable (${target_name} ${ICMPTEST_SRC}) 

yasio_config_app_depends(${target_name})
