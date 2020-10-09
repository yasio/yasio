# A cross platform socket APIs, support ios & android & wp8 & window store universal app.
# Copyright (c) 2012-2020 HALX99.
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 2.8)

# The workaround for solve try_compile failed with code sign
# since cmake-3.18.2, not required
# everyting for cmake toolchain config before project(xxx) is better
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED"
    "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)

# Fix compile failed with armv7 deployment target >= 11.0, xcode clang will report follow error
# clang: error: invalid iOS deployment version '--target=armv7-apple-ios13.6', 
#        iOS 10 is the maximum deployment target for 32-bit targets
# If not defined CMAKE_OSX_DEPLOYMENT_TARGET, cmake will choose latest deployment target
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    if("${CMAKE_OSX_ARCHITECTURES}" MATCHES ".*armv7.*")
        if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET 
        OR "${CMAKE_OSX_DEPLOYMENT_TARGET}" VERSION_GREATER "11.0" 
        OR "${CMAKE_OSX_DEPLOYMENT_TARGET}" VERSION_EQUAL "11.0")
            message(STATUS "Sets iOS minimum deployment target to 10.0 for armv7")
            # a. armv7 maximum deployment 10.x
            # b. armv7 TLS require deployment 10.x
            set(CMAKE_OSX_DEPLOYMENT_TARGET "10.0" CACHE STRING "Minimum OS X deployment version")
        endif()
    else()
        if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
            message(STATUS "The CMAKE_OSX_DEPLOYMENT_TARGET not defined, sets iOS minimum deployment target to 9.0")
            set(CMAKE_OSX_DEPLOYMENT_TARGET "9.0" CACHE STRING "Minimum OS X deployment version")
        endif()
    endif()
endif()

project(yasio)

if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD /O2" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /Od /Zi" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Bv" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Od /Zi /Bv" CACHE STRING "")
endif ()

#option ( LUA_COMPAT_MODULE "compat module" ON)
option(YASIO_BUILD_WITH_SSL "Build yasio with ssl support" OFF)
option(YASIO_BUILD_WITH_CARES "Build yasio with c-ares support" OFF)
option(YASIO_BUILD_WITH_KCP "Build yasio with kcp support" OFF)
option(YASIO_BUILD_WITH_LUA "Build yasio with lua support" OFF)
option(YASIO_BUILD_WITH_CCLUA "Build yasio with cclua support" OFF)
option(YASIO_BUILD_TESTS "Build yasio tests and examples" ON)  
option(YASIO_BUILD_LUA_EXAMPLE "Build yasio lua example" OFF)
option(YASIO_BUILD_AS_SHARED "Build yasio as shared library" OFF)


if (NOT YASIO_BUILD_CXX_STD)
    set(YASIO_BUILD_CXX_STD 11)
endif()

MARK_AS_ADVANCED(YASIO_PROJECT_DIR)

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

# --- The yasio optional dependencies

### openssl support
if (YASIO_BUILD_WITH_SSL)
   add_subdirectory(external/openssl)
endif()

### c-ares support
if (YASIO_BUILD_WITH_CARES)
    set(CARES_STATIC ON CACHE BOOL "Build c-ares as static library" FORCE)
    set(CARES_SHARED OFF CACHE BOOL "Build c-ares as shared library" FORCE)
    set(CARES_BUILD_TOOLS OFF CACHE BOOL "Build c-ares tools" FORCE)
    add_subdirectory(external/c-ares)
    set(CARES_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/c-ares/include" CACHE INTERNAL "c-ares Include Directory" )
endif()

### The yasio core library project
set (YASIO_CORE
    yasio/xxsocket.hpp
    yasio/yasio.hpp
    yasio/ibstream.hpp
    yasio/obstream.hpp
    yasio/xxsocket.cpp
    yasio/yasio.cpp
    yasio/ibstream.cpp
    yasio/obstream.cpp
)

if (YASIO_BUILD_WITH_KCP)
  set(YASIO_CORE ${YASIO_CORE} 
    ${CMAKE_SOURCE_DIR}/external/kcp/ikcp.c
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

if(YASIO_BUILD_WITH_SSL OR YASIO_BUILD_WITH_KCP OR YASIO_BUILD_WITH_LUA OR YASIO_BUILD_WITH_CARES)
    include_directories("${CMAKE_SOURCE_DIR}/external")
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
        add_library(yasio STATIC
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
	    
        set_xcode_property (yasio IPHONEOS_DEPLOYMENT_TARGET "7.0" "all")
    else ()
        # set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
        add_library(yasio STATIC
            ${YASIO_CORE}
            ${THIRDPART_SRC}
        )
        # set_target_properties ( yasio PROPERTIES BUNDLE TRUE )
    endif ()
elseif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Switch")
    add_library(yasio STATIC
        ${YASIO_CORE}
        ${THIRDPART_SRC}
    )
    target_compile_options(yasio PRIVATE -m64 -mcpu=cortex-a57+fp+simd+crypto+crc -fno-common -fno-short-enums -ffunction-sections -fdata-sections -fPIC -fms-extensions)
elseif ( WIN32 AND NOT CYGWIN )
    add_library(yasio STATIC
            ${YASIO_CORE}
            ${THIRDPART_SRC}
        )
else ( ) # linux system
    if(YASIO_BUILD_AS_SHARED)
        add_library(yasio SHARED
            ${YASIO_CORE}
            ${THIRDPART_SRC}
        )
    else()
        add_library(yasio STATIC
            ${YASIO_CORE}
            ${THIRDPART_SRC}
        )
    endif()
endif ( )
	
if ( WIN32 AND NOT CYGWIN )
    target_link_libraries(yasio
        ws2_32
        ${THIRDPART_LIB} 
    )
else ( )
    target_link_libraries(yasio
        m
        ${THIRDPART_LIB} 
    )
endif ( )

macro(ConfigTargetSSL target_name)
    if (YASIO_BUILD_WITH_SSL)
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
    endif()
endmacro()

# --- The compiler flags
message("-- Building yasio with cpp${YASIO_BUILD_CXX_STD} support")
if ( IOS )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
endif ()
if (NOT WIN32 OR CYGWIN)
    if (YASIO_BUILD_CXX_STD EQUAL 17)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
        if ( IOS )
            # Aligned deallocation function of type 'void (void *, std::align_val_t) noexcept' is only available on iOS 11 or newer
            # most of time, low level malloc will alloc a aligned address for new operator,
            # so it's ok to add -faligned-allocation, certainly, still need find a ios9.0 device
            # to test does it works well?
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -faligned-allocation")
        endif()
    elseif(YASIO_BUILD_CXX_STD EQUAL 14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
else()
    if(YASIO_BUILD_CXX_STD EQUAL 17)
        # target_compile_features(yasio PUBLIC cxx_std_17) works
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
    elseif(YASIO_BUILD_CXX_STD EQUAL 14)
        # target_compile_features(yasio PUBLIC cxx_std_14) not works
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
    endif()
endif()

# checking build system have openssl
if(OPENSSL_INCLUDE_DIR)
   target_compile_definitions(yasio PUBLIC YASIO_HAVE_SSL=1)
   target_include_directories(yasio PRIVATE "${OPENSSL_INCLUDE_DIR}")
   if(YASIO_BUILD_AS_SHARED AND (ANDROID OR LINUX))
       target_link_libraries (yasio ext_ssl ext_crypto)
   endif()
endif()

# checking build system have c-ares
if(CARES_INCLUDE_DIR AND YASIO_BUILD_WITH_CARES)
   target_include_directories(yasio PRIVATE "${CARES_INCLUDE_DIR}")
   target_link_libraries(yasio c-ares)
   target_compile_definitions(yasio PUBLIC YASIO_HAVE_CARES=1 YASIO_ENABLE_ARES_PROFILER=1)

   if(ANDROID AND YASIO_BUILD_AS_SHARED)
       target_compile_definitions(yasio PRIVATE YASIO_BUILD_AS_SHARED=1)
   endif()
endif()

# checking build system have kcp
if(YASIO_BUILD_WITH_KCP)
    target_compile_definitions(yasio PUBLIC YASIO_HAVE_KCP=1)
endif()

if (ANDROID AND ANDROID_STRIP_TOOL)
    add_custom_command(TARGET yasio POST_BUILD
        COMMAND ${ANDROID_STRIP_TOOL} ${PROJECT_BINARY_DIR}/libyasio.so -o ${PROJECT_BINARY_DIR}/libyasio.so
    )
endif()

# The tests & examples
if(NOT IOS AND YASIO_BUILD_TESTS)
    add_subdirectory(tests/tcp)
    add_subdirectory(tests/mcast)
    if(YASIO_BUILD_WITH_KCP)
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
    if(YASIO_BUILD_LUA_EXAMPLE)
        add_subdirectory(examples/lua)
        target_include_directories(example_lua PRIVATE external)
        target_compile_definitions(example_lua PRIVATE YASIO_LUA_ENABLE_GLOBAL=1)
    endif()
    add_subdirectory(examples/ftp_server)

    if(YASIO_BUILD_WITH_SSL)
        add_subdirectory(tests/ssl)
    endif()
endif ()

### Sets startup project for vs .sln
if (WIN32 AND YASIO_BUILD_TESTS)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "ftp_server")
endif ()
