set(target_name issue377)

set (ISSUE377_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE377_INC_DIR ${ISSUE377_SRC_DIR}/../../)

set (ISSUE377_SRC ${ISSUE377_SRC_DIR}/main.cpp)


include_directories ("${ISSUE377_SRC_DIR}")
include_directories ("${ISSUE377_INC_DIR}")

add_executable (${target_name} ${ISSUE377_SRC}) 

yasio_config_app_depends(${target_name})
