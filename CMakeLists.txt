# A multi-platform support c++11 library with focus on asynchronous socket I/O for any client application.
#
# Copyright (c) 2012-2024 HALX99.
#
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 3.13.0)

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Debug)
endif()

cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0048 NEW)

set(yasio_target_name yasio)
project(${yasio_target_name})

set(_1kfetch_cache_dir "${CMAKE_CURRENT_LIST_DIR}/cache" CACHE STRING "" FORCE)
set(_1kfetch_manifest "${CMAKE_CURRENT_LIST_DIR}/manifest.json" CACHE STRING "" FORCE)

include(1k/platform.cmake)
include(1k/fetch.cmake)
include(CMakeDependentOption)

cmake_dependent_option(YASIO_BUILD_TESTS
  "Build the tests when we are the root project" ON
  "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

cmake_dependent_option(YASIO_BUILD_LUA_EXAMPLE
  "Build lua example when we are the root project" ON
  "CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR" OFF)

option(YASIO_ENABLE_EXT_HTTP "Build yasio http extension" ON)
option(YASIO_ENABLE_LUA "Build yasio with lua support" OFF)
option(YASIO_ENABLE_AXLUA "Build yasio with axmol-lua support" OFF)
option(YASIO_ENABLE_NI "Build yasio with native interface for interop" OFF)
option(YASIO_ENABLE_KCP "Enable kcp support" OFF)
option(YASIO_NO_DEPS "Build yasio without deps" OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(YASIO_ROOT ${CMAKE_CURRENT_LIST_DIR})

message(STATUS "YASIO_ROOT=${YASIO_ROOT}")
message(STATUS "CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}")
message(STATUS "MSVC=${MSVC}, MSVC_IDE=${MSVC_IDE}")

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(FULL_MSVC TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(FULL_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(FULL_GCC TRUE)
endif()

# --- The C/C++ standard
if (NOT MSVC)
    if (NOT CXX_STD)
        set(CXX_STD 11)
    endif()
    set(CMAKE_CXX_STANDARD ${CXX_STD})
else()
    if (CXX_STD)
        set(CMAKE_CXX_STANDARD ${CXX_STD})
    else()
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("/std:c++latest" YASIO_HAVE_CXX_LATEST)
        if (YASIO_HAVE_CXX_LATEST)
            set(CXX_STD latest)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
        else()
            set(CXX_STD 11)
        endif()
    endif()
endif()

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED ON)

message(STATUS "Building yasio with C++${CXX_STD} support")
message(STATUS "CMAKE_SYSTEM_NAME=" "${CMAKE_SYSTEM_NAME}")

function(yasio_config_target_outdir target)
    set_target_properties(${target} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
endfunction()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    if(ANDROID)
        set(SYSTEM_STRING "Android")
    else()
        set(SYSTEM_STRING "Linux")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    set(UWP TRUE)
endif()

if (WIN32)
    if(FULL_GCC)
        set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
    else()
        add_definitions(-D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE -D_SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)
        remove_definitions(-D_MBCS)
    endif()
    if (FULL_MSVC)
        add_compile_options(/Bv /GF)
    else()
        add_definitions(-D__STDC_FORMAT_MACROS)
    endif()
endif()

# Auto choose ssl backend if not supplied by user
if (NOT DEFINED YASIO_SSL_BACKEND)
    if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
        set(YASIO_SSL_BACKEND 0)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "watchOS")
        set(YASIO_SSL_BACKEND 2) # mbedtls
    else()
        set(YASIO_SSL_BACKEND 1) # openssl
    endif()
endif()

message(STATUS "YASIO_SSL_BACKEND=${YASIO_SSL_BACKEND}")

if (NOT DEFINED YASIO_ENABLE_HALF_FLOAT)
    set(YASIO_ENABLE_HALF_FLOAT ON)
endif()

# The compiler flags
if(BUILD_SHARED_LIBS AND LINUX)
   set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -fPIC")
endif()

set (YASIO_USE_PREBUILT_LUA FALSE)

if (ANDROID)
    if (NOT THIRDPART_LIB)
        set(THIRDPART_LIB "-llog")
    else()
        set(THIRDPART_LIB "${THIRDPART_LIB} -llog")
    endif()
endif()

set(YASIO_INC_DIRS 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty"
)

### no deps
if (YASIO_NO_DEPS)
    set(YASIO_SSL_BACKEND 0)
    set(YASIO_USE_CARES OFF)
    set(YASIO_ENABLE_HPERF_IO OFF)
    set(YASIO_ENABLE_HALF_FLOAT OFF)
    set(YASIO_ENABLE_LUA OFF CACHE BOOL "" FORCE)
    set(YASIO_ENABLE_EXT_HTTP OFF CACHE BOOL "" FORCE)
    set(YASIO_BUILD_LUA_EXAMPLE OFF CACHE BOOL "" FORCE)
endif()

### ssl support

if (YASIO_SSL_BACKEND EQUAL 1) # openssl
   message(STATUS "Build with openssl support")
   add_subdirectory(3rdparty/openssl)
elseif(YASIO_SSL_BACKEND EQUAL 2) # mbedtls
    message(STATUS "Build with mbedtls support")
    set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbedtls programs" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "Build mbed TLS tests." FORCE)
    set(MBEDTLS_VER "3.4.1")
    if(MSVC_VERSION LESS 1900)
        message(AUTHOR_WARNING "Using mbedtls 2.28.3 for vs2013 happy")
        set(MBEDTLS_VER "2.28.3")
    endif()
    include(1k/CPM.cmake)
    CPMAddPackage(
      NAME mbedtls
      VERSION ${MBEDTLS_VER}
      GITHUB_REPOSITORY "Mbed-TLS/mbedtls"
      GIT_TAG "v${MBEDTLS_VER}"
      OPTIONS
        "MBEDTLS_FATAL_WARNINGS OFF"
    )
    yasio_config_target_outdir(mbedtls)
    yasio_config_target_outdir(mbedcrypto)
    yasio_config_target_outdir(mbedx509)
endif()

### c-ares support
if (YASIO_USE_CARES)
    add_subdirectory(3rdparty/c-ares)
endif()

### The yasio core library project
file(GLOB YASIO_CORE yasio/*.hpp;yasio/*.cpp;yasio/compiler/*.hpp;yasio/impl/*.hpp;)

if (YASIO_ENABLE_KCP)
  if (NOT kcp_SOURCE_DIR)
      set(YASIO_CORE ${YASIO_CORE} 
        ${PROJECT_SOURCE_DIR}/3rdparty/kcp/ikcp.c
      )
      list(APPEND YASIO_INC_DIRS  "${PROJECT_SOURCE_DIR}/3rdparty/kcp")
  else()
      list(APPEND THIRDPART_LIB "kcp")
  endif()
endif()
if (YASIO_ENABLE_NI)
  set(YASIO_CORE ${YASIO_CORE} 
    yasio/bindings/yasio_ni.cpp
  )
endif()

if (YASIO_ENABLE_LUA)
  set(YASIO_CORE ${YASIO_CORE} 
    yasio/bindings/lyasio.cpp
  )
  
  if(YASIO_ENABLE_AXLUA)
      set(YASIO_CORE ${YASIO_CORE} 
        yasio/bindings/yasio_axlua.cpp
      )
  endif()
endif()

if(ANDROID AND CARES_INCLUDE_DIR)
  set(YASIO_CORE ${YASIO_CORE} 
    yasio/platform/yasio_jni.cpp
  )
endif()

if(WIN32 AND YASIO_ENABLE_HPERF_IO)
  set(THIRDPART_SRC 3rdparty/wepoll/wepoll.c)
endif()

if(YASIO_ENABLE_KCP)
    include_directories("${PROJECT_SOURCE_DIR}/3rdparty")
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

if ( WIN32 )
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

message(STATUS "YASIO_INC_DIRS=${YASIO_INC_DIRS}")
target_include_directories(${yasio_target_name} PUBLIC ${YASIO_INC_DIRS})

if (BUILD_SHARED_LIBS)
    target_compile_definitions(${yasio_target_name} 
        PUBLIC  YASIO_BUILD_SHARED_LIBS=1 # Tell yasio we are building shared libs
        PUBLIC  YASIO_EXPORT_DLL=1 # Tell yasio to we needs export all yasio-core classes
        PRIVATE YASIO_LIB=1 # Tell yasio we are building yasio-core libs
        )
endif()

macro(yasio_link_ssl_libraries target_name)
    if (YASIO_SSL_BACKEND EQUAL 1)
        if (UNIX)
            target_link_libraries (${target_name} OpenSSL::SSL OpenSSL::Crypto dl)
        else ()
            target_link_libraries (${target_name} OpenSSL::SSL OpenSSL::Crypto)
        endif()
    elseif(YASIO_SSL_BACKEND EQUAL 2)
        target_link_libraries(${target_name} mbedtls mbedcrypto mbedx509)
    endif()
endmacro()

macro(yasio_config_lib_depends target_name)
    target_link_libraries(${target_name} ${yasio_target_name})
    if ((NOT WIN32) AND (NOT ANDROID))
        target_link_libraries(${target_name} pthread)
    endif()
    
    yasio_link_ssl_libraries(${target_name})
    
    # copy openssl dlls
    if (MSVC AND YASIO_SSL_BACKEND EQUAL 1)
        get_property(SSL_DLL_PATH TARGET OpenSSL::SSL PROPERTY IMPORTED_LOCATION)
        get_property(CRYPTO_DLL_PATH TARGET OpenSSL::Crypto PROPERTY IMPORTED_LOCATION)
        add_custom_command(TARGET ${target_name}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SSL_DLL_PATH}
            ${CRYPTO_DLL_PATH}
            $<TARGET_FILE_DIR:${target_name}>
        )
    endif ()
    
    # copy yasio.dll
    if(BUILD_SHARED_LIBS AND WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/\$\(Configuration\)/yasio.dll"
                $<TARGET_FILE_DIR:${target_name}>
            )
        # copy plainlua.dll
        if(YASIO_ENABLE_LUA)
            if(NOT YASIO_USE_PREBUILT_LUA)
                add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${CMAKE_BINARY_DIR}/\$\(Configuration\)/plainlua.dll"
                        $<TARGET_FILE_DIR:${target_name}>
                    )
            endif()
        endif()
    endif()
endmacro()

macro(yasio_config_app_folder target_name)
    set_target_properties(${target_name} PROPERTIES 
        FOLDER "apps")
endmacro()

macro(yasio_config_app_depends target_name)
    yasio_config_lib_depends(${target_name})
    yasio_config_app_folder(${target_name})
endmacro(yasio_config_app_depends)

macro(yasio_config_http_app_depends target_name)
    yasio_config_app_depends(${target_name})
    target_link_libraries (${target_name} yasio_http)
endmacro(yasio_config_http_app_depends)

# checking build system have openssl
if(OPENSSL_INCLUDE_DIR AND (YASIO_SSL_BACKEND EQUAL 1))
    message(STATUS "OPENSSL_INCLUDE_DIR=" ${OPENSSL_INCLUDE_DIR})
    target_include_directories(${yasio_target_name} PRIVATE "${OPENSSL_INCLUDE_DIR}")
endif()

# checking build system have c-ares
if(CARES_INCLUDE_DIR AND YASIO_USE_CARES)
   target_include_directories(${yasio_target_name} PRIVATE "${CARES_INCLUDE_DIR}")
   target_link_libraries(${yasio_target_name} c-ares)
endif()

if (BUILD_SHARED_LIBS AND ANDROID)
    message(STATUS "CMAKE_STRIP=${CMAKE_STRIP}")
    add_custom_command(TARGET ${yasio_target_name} POST_BUILD
        COMMAND mkdir -p ${PROJECT_BINARY_DIR}/stripped && ${CMAKE_STRIP} ${PROJECT_BINARY_DIR}/libyasio.so -o ${PROJECT_BINARY_DIR}/stripped/libyasio.so
    )
endif()

# lua
if(YASIO_ENABLE_LUA)
    find_package(Lua "5.1.0")
    if(NOT LUA_FOUND)
        add_subdirectory(yasio/bindings/lua)
        yasio_config_target_outdir(plainlua)
        target_link_libraries(${yasio_target_name} plainlua)
        set(YASIO_USE_PREBUILT_LUA FALSE)
    else()
        target_include_directories(${yasio_target_name} PUBLIC ${LUA_INCLUDE_DIR})
        target_link_libraries(${yasio_target_name} ${LUA_LIBRARIES})
        set(YASIO_USE_PREBUILT_LUA TRUE)
    endif()

    if(MSVC)
        target_compile_options(${yasio_target_name} PRIVATE /bigobj)
    endif()
endif()

# link libraries for yasio_core when BUILD_SHARED_LIBS=TRUE
yasio_link_ssl_libraries(${yasio_target_name})
if(BUILD_SHARED_LIBS)
    if(NOT WIN32 AND (NOT ANDROID))
        target_link_libraries(${yasio_target_name} pthread)
    endif()
endif()

# The yasio config preprocessors
macro(yasio_config_pred target_name pred)
    if(${pred})
        target_compile_definitions(${target_name} PUBLIC ${pred}=1)
    endif()
endmacro()
macro(yasio_config_option target_name optname optval)
    if(${optname})
        target_compile_definitions(${target_name} PUBLIC ${optname}=${optval})
    endif()
endmacro()

macro (yasio_config_lib_options target_name)
    yasio_config_pred(${target_name} YASIO_VERBOSE_LOG)
    yasio_config_pred(${target_name} YASIO_USE_SPSC_QUEUE)
    yasio_config_pred(${target_name} YASIO_USE_SHARED_PACKET)
    yasio_config_pred(${target_name} YASIO_USE_CARES)
    yasio_config_pred(${target_name} YASIO_DISABLE_OBJECT_POOL)
    yasio_config_pred(${target_name} YASIO_ENABLE_ARES_PROFILER)
    yasio_config_pred(${target_name} YASIO_ENABLE_KCP)
    yasio_config_option(${target_name} YASIO_SSL_BACKEND "${YASIO_SSL_BACKEND}")
    yasio_config_pred(${target_name} YASIO_ENABLE_UDS)
    yasio_config_pred(${target_name} YASIO_NT_COMPAT_GAI)
    yasio_config_pred(${target_name} YASIO_MINIFY_EVENT)
    yasio_config_pred(${target_name} YASIO_ENABLE_HALF_FLOAT)
    yasio_config_pred(${target_name} YASIO_ENABLE_PASSIVE_EVENT)
    yasio_config_pred(${target_name} YASIO_NO_JNI_ONLOAD)
    yasio_config_pred(${target_name} YASIO_ENABLE_HPERF_IO)
    yasio_config_pred(${target_name} YASIO_DISABLE_POLL)
    yasio_config_pred(${target_name} YASIO_DISABLE_EPOLL)
    yasio_config_pred(${target_name} YASIO_DISABLE_KQUEUE)
    yasio_config_pred(${target_name} YASIO_NT_XHRES_TIMER)
    yasio_config_target_outdir(${yasio_target_name})
endmacro()

macro (yasio_config_ext_options target_name)
    yasio_config_lib_options(${target_name})
    target_include_directories(${target_name} PUBLIC "${YASIO_ROOT}/extensions")
endmacro()

yasio_config_lib_options(${yasio_target_name})

# see also: https://docs.microsoft.com/en-us/visualstudio/debugger/create-custom-views-of-native-objects
if (MSVC_IDE)
  target_sources(${yasio_target_name} PRIVATE yasio/yasio.natvis)
endif()

if(YASIO_SSL_BACKEND)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_LIST_DIR}/certs/cacert.pem,${CMAKE_CURRENT_LIST_DIR}/certs/ca-cer.pem" SSLTEST_CACERT)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_LIST_DIR}/certs/server-cer.pem" SSLTEST_CERT)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_LIST_DIR}/certs/server-prk.pem" SSLTEST_PKEY)
    # ${CMAKE_BINARY_DIR} will be set include for all targets by cmake auto
    configure_file("${CMAKE_CURRENT_LIST_DIR}/certs/sslcerts.hpp.in" "${CMAKE_BINARY_DIR}/sslcerts.hpp" @ONLY)
    target_include_directories(${yasio_target_name} INTERFACE ${CMAKE_BINARY_DIR})
endif()

if(YASIO_ENABLE_EXT_HTTP)
    # llhttp for extensions: yasio_http, ...
    if (NOT BUILD_SHARED_LIBS)
        set (BUILD_SHARED_LIBS FALSE CACHE BOOL "" FORCE)
        set (BUILD_STATIC_LIBS TRUE CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(3rdparty/llhttp)

    # yasio_http
    add_subdirectory(extensions/yasio_http)
    target_link_libraries(yasio_http llhttp::llhttp)
    yasio_config_ext_options(yasio_http)
endif()

# The tests & examples
if(YASIO_BUILD_TESTS)
    add_subdirectory(tests/tcp)
    add_subdirectory(tests/icmp)
    add_subdirectory(tests/mcast)
    add_subdirectory(tests/speed)
    add_subdirectory(tests/mtu)
    add_subdirectory(tests/issue166)
    add_subdirectory(tests/issue178)
    add_subdirectory(tests/issue201)
    add_subdirectory(tests/issue208)
    add_subdirectory(tests/issue245)
    add_subdirectory(tests/issue256)
    add_subdirectory(tests/issue377)
    add_subdirectory(tests/issue384)
    add_subdirectory(tests/echo_server)
    add_subdirectory(tests/echo_client)
    if(YASIO_ENABLE_LUA AND YASIO_BUILD_LUA_EXAMPLE)
        add_subdirectory(examples/lua)
        target_include_directories(example_lua PRIVATE 3rdparty)
        target_compile_definitions(example_lua PRIVATE YASIO_LUA_ENABLE_GLOBAL=0)
    endif()
    if (NOT YASIO_NO_DEPS)
        add_subdirectory(examples/ftp_server)
        add_subdirectory(tests/http)
    endif()
    if (YASIO_SSL_BACKEND)
        add_subdirectory(tests/ssl)
    endif()
endif ()

### Sets startup project for vs .sln
if (WIN32 AND YASIO_BUILD_TESTS)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "icmptest")
endif ()
