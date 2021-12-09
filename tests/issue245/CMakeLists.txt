set (target_name issue245)

set (ISSUE245_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE245_INC_DIR ${ISSUE245_SRC_DIR}/../../)

set (ISSUE245_SRC ${ISSUE245_SRC_DIR}/main.cpp)

include_directories ("${ISSUE245_SRC_DIR}")
include_directories ("${ISSUE245_INC_DIR}")

add_executable (${target_name} ${ISSUE245_SRC}) 

yasio_config_app_depends(${target_name})
