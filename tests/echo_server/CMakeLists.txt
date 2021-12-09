set(target_name echo_server)

set (ECHO_SERVER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (ECHO_SERVER_INC_DIR ${ECHO_SERVER_SRC_DIR}/../../)

set (ECHO_SERVER_SRC ${ECHO_SERVER_SRC_DIR}/main.cpp)

include_directories ("${ECHO_SERVER_SRC_DIR}")
include_directories ("${ECHO_SERVER_INC_DIR}")

add_executable (${target_name} ${ECHO_SERVER_SRC}) 

yasio_config_app_depends(${target_name})
