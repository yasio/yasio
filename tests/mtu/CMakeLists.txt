set(target_name mtutest)

set (MTUTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (MTUTEST_INC_DIR ${MTUTEST_SRC_DIR}/../../)

set (TCPTEST_SRC ${MTUTEST_SRC_DIR}/main.cpp)


include_directories ("${MTUTEST_SRC_DIR}")
include_directories ("${MTUTEST_INC_DIR}")

add_executable (${target_name} ${TCPTEST_SRC}) 

yasio_config_app_depends(${target_name})
