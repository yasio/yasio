set (EXAMPLE_LUA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LUA_SRC_PATH ${EXAMPLE_LUA_SRC_DIR}/lua)

file(GLOB LUA_SRC_FILES 
    "${LUA_SRC_PATH}/*.c")

list(REMOVE_ITEM LUA_SRC_FILES "${LUA_SRC_PATH}/lua.c")

set (EXAMPLE_LUA_SRC ${EXAMPLE_LUA_SRC_DIR}/main.cpp
    ${EXAMPLE_LUA_SRC_DIR}/../../yasio/bindings/lyasio.cpp
)

if (NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

set (EXAMPLE_LUA_INC_DIR ${EXAMPLE_LUA_SRC_DIR}/../../)

include_directories (
    "${EXAMPLE_LUA_SRC_DIR}"
    "${EXAMPLE_LUA_INC_DIR}"
    ${LUA_SRC_PATH}
    )

add_executable (example_lua 
    ${EXAMPLE_LUA_SRC} 
    ${LUA_SRC_FILES}
) 

if (MSVC)
    set (EXAMPLE_LUA_LDLIBS yasio)
else ()
    set (EXAMPLE_LUA_LDLIBS yasio pthread)
endif()

target_link_libraries (example_lua ${EXAMPLE_LUA_LDLIBS})

if (MSVC)
    set_target_properties(example_lua PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:example_lua>")
endif ()

add_custom_command(TARGET example_lua
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts
    $<TARGET_FILE_DIR:example_lua>/scripts/
    )

if(MSVC)
    add_custom_command(TARGET example_lua
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/example_lua.xsxproj
        $<TARGET_FILE_DIR:example_lua>/example_lua.xsxproj
        )
endif()
