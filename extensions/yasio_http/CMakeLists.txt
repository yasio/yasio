
set(target_name yasio_http)

FILE(GLOB_RECURSE YASIO_HTTP_SOURCES *.h;*.cpp;*.c)

add_library(${target_name} STATIC ${YASIO_HTTP_SOURCES})

yasio_config_lib_depends(${target_name})

target_include_directories(${target_name} 
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" 
    PUBLIC "${YASIO_ROOT}/thirdparty/llhttp/include"
)
