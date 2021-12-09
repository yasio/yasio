set (target_name mcasttest2)

set (MCASTTEST2_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (MCASTTEST2_INC_DIR ${MCASTTEST2_SRC_DIR}/../../)

set (MCASTTEST2_SRC ${MCASTTEST2_SRC_DIR}/main.cpp)

include_directories ("${MCASTTEST2_SRC_DIR}")
include_directories ("${MCASTTEST2_INC_DIR}")

add_executable (${target_name} ${MCASTTEST2_SRC}) 

yasio_config_app_depends(${target_name})
