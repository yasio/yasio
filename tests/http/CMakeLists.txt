set (target_name httptest)

set (HTTPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (HTTPTEST_INC_DIR ${HTTPTEST_SRC_DIR}/../../)

set (HTTPTEST_SRC ${HTTPTEST_SRC_DIR}/main.cpp)

include_directories ("${HTTPTEST_SRC_DIR}")
include_directories ("${HTTPTEST_INC_DIR}")

add_executable (${target_name} ${HTTPTEST_SRC})
yasio_config_http_app_depends(${target_name})

