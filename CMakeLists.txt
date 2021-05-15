# A multi-platform support c++11 library with focus on asynchronous socket I/O for any 
# client application.
# Copyright (c) 2012-2021 HALX99.
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 3.13.0)

cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0048 NEW)

set(yasio_target_name yasio)
project(${yasio_target_name})

if (WIN32)
    add_compile_options(" /Bv")
endif()

#option ( LUA_COMPAT_MODULE "compat module" ON)
option(YASIO_BUILD_WITH_LUA "Build yasio with lua support" OFF)
option(YASIO_BUILD_WITH_CCLUA "Build yasio with cclua support" OFF)
option(YASIO_BUILD_TESTS "Build yasio tests and examples" ON)  
option(YASIO_BUILD_LUA_EXAMPLE "Build yasio lua example" ON)
option(YAISO_BUILD_NI "Build yasio with native interface for interop" OFF)


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (LINUX TRUE)
endif()

if (NOT DEFINED YASIO_SSL_BACKEND)
    set(YASIO_SSL_BACKEND 0)
endif()

if (NOT DEFINED YASIO_HAVE_HALF_FLOAT)
    set(YASIO_HAVE_HALF_FLOAT ON)
endif()

if (NOT CXX_STD)
    set(CXX_STD 11)
endif()

MARK_AS_ADVANCED(YASIO_PROJECT_DIR)

set (YASIO_USE_PREBUILT_LUA FALSE)

if (ANDROID)
    if (NOT THIRDPART_LIB)
        set(THIRDPART_LIB "-llog")
    else()
        set(THIRDPART_LIB "${THIRDPART_LIB} -llog")
    endif()
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${THIRDPART_INC}
)

### openssl support
if (YASIO_SSL_BACKEND EQUAL 1) # openssl
   add_subdirectory(external/openssl)
elseif(YASIO_SSL_BACKEND EQUAL 2) # mbedtls
    include_directories("${PROJECT_SOURCE_DIR}/external/mbedtls/include")
    set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbedtls programs" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "Build mbed TLS tests." FORCE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/external/mbedtls)
endif()

### c-ares support
if (YASIO_HAVE_CARES)
    set(CARES_STATIC ON CACHE BOOL "Build c-ares as static library" FORCE)
    set(CARES_SHARED OFF CACHE BOOL "Build c-ares as shared library" FORCE)
    set(CARES_BUILD_TOOLS OFF CACHE BOOL "Build c-ares tools" FORCE)
    if(LINUX AND BUILD_SHARED_LIBS)
        set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -O3 -fPIC")
    endif()
    add_subdirectory(external/c-ares)
    set(CARES_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/external/c-ares/include" CACHE INTERNAL "c-ares Include Directory" )
endif()

### The yasio core library project
set (YASIO_CORE
    yasio/detail/obstream.hpp
    yasio/detail/ibstream.hpp
    yasio/xxsocket.hpp
    yasio/yasio.hpp
    yasio/xxsocket.cpp
    yasio/yasio.cpp
)

if (YASIO_HAVE_KCP)
  set(YASIO_CORE ${YASIO_CORE} 
    ${PROJECT_SOURCE_DIR}/external/kcp/ikcp.c
  )
endif()

if (YAISO_BUILD_NI)
  set(YASIO_CORE ${YASIO_CORE} 
    yasio/bindings/yasio_ni.cpp
  )
endif()

if (YASIO_BUILD_WITH_LUA)
  set(YASIO_CORE ${YASIO_CORE} 
    yasio/bindings/lyasio.cpp
  )
  
  if(YASIO_BUILD_WITH_CCLUA)
      set(YASIO_CORE ${YASIO_CORE} 
        yasio/bindings/yasio_cclua.cpp
      )
  endif()
endif()

if(ANDROID AND CARES_INCLUDE_DIR)
  set(YASIO_CORE ${YASIO_CORE} 
    yasio/platform/yasio_jni.cpp
  )
endif()

if(YASIO_HAVE_KCP OR YASIO_BUILD_WITH_LUA OR YASIO_HAVE_CARES OR YASIO_HAVE_HALF_FLOAT)
    include_directories("${PROJECT_SOURCE_DIR}/external")
endif()

macro(source_group_by_dir proj_dir source_files)
    if(MSVC OR APPLE)
        get_filename_component(sgbd_cur_dir ${proj_dir} ABSOLUTE)
        foreach(sgbd_file ${${source_files}})
			get_filename_component(sgbd_abs_file ${sgbd_file} ABSOLUTE)
            file(RELATIVE_PATH sgbd_fpath ${sgbd_cur_dir} ${sgbd_abs_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC OR APPLE)
endmacro(source_group_by_dir)

source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} YASIO_CORE)

# --- add yasio core lib project
if (APPLE)
    if (IOS)
        # set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
        add_library(${yasio_target_name} STATIC
           ${YASIO_CORE}
           ${THIRDPART_SRC}
        )
        
        macro(set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE XCODE_RELVERSION)
            set(XCODE_RELVERSION_I "${XCODE_RELVERSION}")
            if(XCODE_RELVERSION_I STREQUAL "All")
                set_property(TARGET ${TARGET} PROPERTY
                XCODE_ATTRIBUTE_${XCODE_PROPERTY} "${XCODE_VALUE}")
            else()
               set_property(TARGET ${TARGET} PROPERTY
               XCODE_ATTRIBUTE_${XCODE_PROPERTY}[variant=${XCODE_RELVERSION_I}] "${XCODE_VALUE}")
            endif()
        endmacro(set_xcode_property)
	    
        set_xcode_property (${yasio_target_name} IPHONEOS_DEPLOYMENT_TARGET "7.0" "all")
    else ()
        # set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
        if (BUILD_SHARED_LIBS)
            add_library(${yasio_target_name} MODULE
                ${YASIO_CORE}
                ${THIRDPART_SRC}
            )
            set_target_properties (${yasio_target_name} PROPERTIES BUNDLE TRUE )
        else()
            add_library(${yasio_target_name} STATIC
                ${YASIO_CORE}
                ${THIRDPART_SRC}
            )
        endif()
    endif ()
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Switch")
    add_library(${yasio_target_name} STATIC
        ${YASIO_CORE}
        ${THIRDPART_SRC}
    )
    target_compile_options(${yasio_target_name} PRIVATE -m64 -mcpu=cortex-a57+fp+simd+crypto+crc -fno-common -fno-short-enums -ffunction-sections -fdata-sections -fPIC -fms-extensions)
else ( ) # linux/win32/android
    add_library(${yasio_target_name}
        ${YASIO_CORE}
        ${THIRDPART_SRC}
    )
endif ( )
	
if ( WIN32 AND NOT CYGWIN )
    target_link_libraries(${yasio_target_name}
        ws2_32
        ${THIRDPART_LIB} 
    )
else ( )
    target_link_libraries(${yasio_target_name}
        m
        ${THIRDPART_LIB} 
    )
endif ( )

target_include_directories(${yasio_target_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/external)

if (BUILD_SHARED_LIBS)
    target_compile_definitions(${yasio_target_name} 
        PUBLIC  YASIO_BUILD_AS_SHARED=1 
        PRIVATE YASIO_LIB=1
        )
endif()

macro(ConfigTargetDepends target_name)
    # config ssl dll or lib
    if (YASIO_SSL_BACKEND EQUAL 1)
        if (UNIX)
            target_link_libraries (${target_name} ext_ssl ext_crypto dl)
        else ()
            target_link_libraries (${target_name} ext_ssl ext_crypto)
        endif()

        if (WIN32)
            add_custom_command(TARGET ${target_name}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${OPENSSL_LIBRARY_DIR}/libcrypto-1_1.dll
                ${OPENSSL_LIBRARY_DIR}/libssl-1_1.dll
                $<TARGET_FILE_DIR:${target_name}>
            )
        endif()
    elseif(YASIO_SSL_BACKEND EQUAL 2)
        target_link_libraries(${target_name} mbedtls mbedcrypto mbedx509)
    endif()
    
    # copy yasio.dll
    if(BUILD_SHARED_LIBS AND WIN32)
       add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/\$\(Configuration\)/yasio.dll"
                $<TARGET_FILE_DIR:${target_name}>
            )
    endif()
    
    # copy plainlua.dll
    if(YASIO_BUILD_WITH_LUA AND WIN32)
        if(NOT YASIO_USE_PREBUILT_LUA)
            add_custom_command(TARGET ${target_name} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/\$\(Configuration\)/plainlua.dll"
                    $<TARGET_FILE_DIR:${target_name}>
                )
        endif()
    endif()
endmacro()

# --- The compiler flags
message("-- Building yasio with cpp${CXX_STD} support")
if ( IOS )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
endif ()
if (NOT WIN32 OR CYGWIN)
    if (CXX_STD EQUAL 17)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
        if ( IOS )
            # Aligned deallocation function of type 'void (void *, std::align_val_t) noexcept' is only available on iOS 11 or newer
            # most of time, low level malloc will alloc a aligned address for new operator,
            # so it's ok to add -faligned-allocation, certainly, still need find a ios9.0 device
            # to test does it works well?
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -faligned-allocation")
        endif()
    elseif(CXX_STD EQUAL 14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
else()
    if(CXX_STD EQUAL 17)
        # target_compile_features(${yasio_target_name} PUBLIC cxx_std_17) works
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    elseif(CXX_STD EQUAL 14)
        # target_compile_features(${yasio_target_name} PUBLIC cxx_std_14) not works
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
    endif()
endif()

# checking build system have openssl
if(OPENSSL_INCLUDE_DIR)
   set(YASIO_SSL_BACKEND 1)
   target_include_directories(${yasio_target_name} PRIVATE "${OPENSSL_INCLUDE_DIR}")
   if(BUILD_SHARED_LIBS AND (ANDROID OR LINUX))
       target_link_libraries (${yasio_target_name} ext_ssl ext_crypto)
   endif()
endif()

# checking build system have c-ares
if(CARES_INCLUDE_DIR AND YASIO_HAVE_CARES)
   target_include_directories(${yasio_target_name} PRIVATE "${CARES_INCLUDE_DIR}")
   target_link_libraries(${yasio_target_name} c-ares)
endif()

if (ANDROID AND BUILD_SHARED_LIBS)
    message("CMAKE_STRIP=${CMAKE_STRIP}")
    add_custom_command(TARGET ${yasio_target_name} POST_BUILD
        COMMAND mkdir -p ${PROJECT_BINARY_DIR}/stripped && ${CMAKE_STRIP} ${PROJECT_BINARY_DIR}/libyasio.so -o ${PROJECT_BINARY_DIR}/stripped/libyasio.so
    )
endif()

# lua
if(YASIO_BUILD_WITH_LUA)
    find_package(Lua "5.1.0")
    if(NOT LUA_FOUND)
        add_subdirectory(yasio/bindings/lua)
        set_target_properties(plainlua PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        target_link_libraries(${yasio_target_name} plainlua)
        set(YASIO_USE_PREBUILT_LUA FALSE)
    else()
        target_include_directories(${yasio_target_name} PUBLIC ${LUA_INCLUDE_DIR})
        target_link_libraries(${yasio_target_name} ${LUA_LIBRARIES})
        set(YASIO_USE_PREBUILT_LUA TRUE)
    endif()
endif()

# The yasio config preprocessors
macro(yasio_config_pred pred)
    if(${pred})
        target_compile_definitions(${yasio_target_name} PUBLIC ${pred}=1)
    endif()
endmacro()
macro(yasio_config_option optname optval)
    if(${optname})
        target_compile_definitions(${yasio_target_name} PUBLIC ${optname}=${optval})
    endif()
endmacro()

yasio_config_pred(YASIO_VERBOSE_LOG)
yasio_config_pred(YASIO_USE_SPSC_QUEUE)
yasio_config_pred(YASIO_USE_SHARED_PACKET)
yasio_config_pred(YASIO_DISABLE_OBJECT_POOL)
yasio_config_pred(YASIO_ENABLE_ARES_PROFILER)
yasio_config_pred(YASIO_HAVE_CARES)
yasio_config_pred(YASIO_HAVE_KCP)
yasio_config_option(YASIO_SSL_BACKEND "${YASIO_SSL_BACKEND}")
# yasio_config_pred(YASIO_DISABLE_CONCURRENT_SINGLETON)
yasio_config_pred(YASIO_ENABLE_UDS)
yasio_config_pred(YASIO_NT_COMPAT_GAI)
yasio_config_pred(YASIO_MINIFY_EVENT)
yasio_config_pred(YASIO_HAVE_HALF_FLOAT)
yasio_config_pred(YASIO_ENABLE_PASSIVE_EVENT)
yasio_config_pred(YASIO_NO_JNI_ONLOAD)

# The tests & examples
if(NOT IOS AND YASIO_BUILD_TESTS)
    add_subdirectory(tests/tcp)
    add_subdirectory(tests/mcast)
    if(YASIO_HAVE_KCP)
        add_subdirectory(tests/speed)
    endif()
    add_subdirectory(tests/issue166)
    add_subdirectory(tests/issue178)
    add_subdirectory(tests/issue201)
    add_subdirectory(tests/issue208)
    add_subdirectory(tests/issue245)
    add_subdirectory(tests/issue256)
    add_subdirectory(tests/echo_server)
    add_subdirectory(tests/echo_client)
    if(YASIO_BUILD_WITH_LUA AND YASIO_BUILD_LUA_EXAMPLE)
        add_subdirectory(examples/lua)
        target_include_directories(example_lua PRIVATE external)
        target_compile_definitions(example_lua PRIVATE YASIO_LUA_ENABLE_GLOBAL=0)
    endif()
    add_subdirectory(examples/ftp_server)

    if(YASIO_SSL_BACKEND)
        add_subdirectory(tests/ssl)
    endif()
endif ()

### Sets startup project for vs .sln
if (WIN32 AND YASIO_BUILD_TESTS)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "ftp_server")
endif ()
