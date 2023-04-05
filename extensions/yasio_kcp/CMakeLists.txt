
set(target_name yasio_kcp)

FILE(GLOB_RECURSE YASIO_KCP_SOURCES *.hpp;*.cpp;*.c)

add_library(${target_name} STATIC ${YASIO_KCP_SOURCES})

yasio_config_lib_depends(${target_name})

target_include_directories(${target_name} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
