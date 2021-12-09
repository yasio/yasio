set(target_name example_lua)

set (EXAMPLE_LUA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (EXAMPLE_LUA_SRC ${EXAMPLE_LUA_SRC_DIR}/main.cpp)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

set (EXAMPLE_LUA_INC_DIR ${EXAMPLE_LUA_SRC_DIR}/../../)

include_directories (
    "${EXAMPLE_LUA_SRC_DIR}"
    "${EXAMPLE_LUA_INC_DIR}"
    ${LUA_SRC_PATH}
    )

add_executable (${target_name} 
    ${EXAMPLE_LUA_SRC} 
    ${LUA_SRC_FILES}
) 

if (MSVC)
    set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target_name}>")
endif ()

add_custom_command(TARGET ${target_name}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts
    $<TARGET_FILE_DIR:${target_name}>/scripts/
    )

if(MSVC)
    add_custom_command(TARGET ${target_name}
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/example_lua.xsxproj
        $<TARGET_FILE_DIR:${target_name}>/example_lua.xsxproj
        )
endif()

yasio_config_app_depends(${target_name})
