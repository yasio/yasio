set(target_name issue384)

set (ISSUE384_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ISSUE384_INC_DIR ${ISSUE384_SRC_DIR}/../../)

set (ISSUE384_SRC ${ISSUE384_SRC_DIR}/main.cpp)


include_directories ("${ISSUE384_SRC_DIR}")
include_directories ("${ISSUE384_INC_DIR}")

add_executable (${target_name} ${ISSUE384_SRC}) 

yasio_config_app_depends(${target_name})
