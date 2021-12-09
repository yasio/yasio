set (target_name issue208)

set (ISSUE208_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE208_INC_DIR ${ISSUE208_SRC_DIR}/../../)

set (ISSUE208_SRC ${ISSUE208_SRC_DIR}/main.cpp)

include_directories ("${ISSUE208_SRC_DIR}")
include_directories ("${ISSUE208_INC_DIR}")

add_executable (${target_name} ${ISSUE208_SRC}) 

yasio_config_app_depends(${target_name})
