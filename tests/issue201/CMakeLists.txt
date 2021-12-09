set (target_name issue201)

set (ISSUE201_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE201_INC_DIR ${ISSUE201_SRC_DIR}/../../)

set (ISSUE201_SRC ${ISSUE201_SRC_DIR}/main.cpp)

include_directories ("${ISSUE201_SRC_DIR}")
include_directories ("${ISSUE201_INC_DIR}")

add_executable (${target_name} ${ISSUE201_SRC}) 

yasio_config_app_depends(${target_name})
