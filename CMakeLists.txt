# A cross platform socket APIs, support ios & android & wp8 & window store universal app.
# Copyright (c) 2012-2020 HALX99.
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 3.10)

if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD /O2" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /Od /Zi" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Od /Zi" CACHE STRING "")
endif ()

project(yasio)

if ( IOS )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
endif ()

#option ( LUA_COMPAT_MODULE "compat module" ON)
option(YASIO_BUILD_WITH_SSL "Build with internal ssl support" OFF)
option(YASIO_BUILD_WITH_CARES "Build with internal c-ares support" OFF)
option(YASIO_BUILD_TESTS "Build yasio tests and examples" ON)

MARK_AS_ADVANCED(YASIO_PROJECT_DIR)

if (NOT WIN32 OR CYGWIN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if (ANDROID)
    if (NOT THIRDPART_LIB)
        set(THIRDPART_LIB "-llog")
    else()
        set(THIRDPART_LIB "${THIRDPART_LIB} -llog")
    endif()
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${THIRDPART_INC}
)

set ( YASIO_CORE
    yasio/xxsocket.hpp
    yasio/yasio.hpp
    yasio/ibstream.hpp
    yasio/obstream.hpp
    yasio/xxsocket.cpp
    yasio/yasio.cpp
    yasio/ibstream.cpp
    yasio/obstream.cpp
    yasio/bindings/yasio_ni.cpp
)

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

if (APPLE)
    if (IOS)
        set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
        add_library(yasio STATIC
           ${YASIO_CORE}
           ${THIRDPART_SRC}
        )
	set_xcode_property (yasio IPHONEOS_DEPLOYMENT_TARGET "7.0" "all")
    else ()
        set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
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
else ( )
    add_library(yasio STATIC
        ${YASIO_CORE}
        ${THIRDPART_SRC}
    )
endif ( )
	
if ( WIN32 AND NOT CYGWIN )
    target_link_libraries(yasio
        ws2_32
        ${THIRDPART_LIB} 
    )
    target_compile_features(yasio PUBLIC cxx_std_17)
else ( )
    target_link_libraries(yasio
        m
        ${THIRDPART_LIB} 
    )
endif ( )

if(NOT IOS AND YASIO_BUILD_TESTS)
    add_subdirectory(tests/tcp)
    add_subdirectory(tests/mcast)
    add_subdirectory(tests/kcp)
    add_subdirectory(tests/issue166)
    add_subdirectory(tests/issue178)
    add_subdirectory(tests/issue201)
    add_subdirectory(tests/udp_echo_server)
    add_subdirectory(examples/lua)
    add_subdirectory(examples/ftp_server)
endif ()

### SSL support
if (YASIO_BUILD_WITH_SSL)
    # target_include_directories
    target_compile_definitions(yasio PUBLIC YASIO_HAVE_SSL=1)

    add_subdirectory(openssl)
    target_link_libraries(yasio crypto ssl)
    if(NOT IOS)
        add_subdirectory(tests/ssl)
    endif()
endif ()

### c-ares support
if (YASIO_BUILD_WITH_CARES)
    target_compile_definitions(yasio PUBLIC YASIO_HAVE_CARES=1)
    set(CARES_STATIC ON CACHE BOOL "Build c-ares as static library" FORCE)
    set(CARES_SHARED OFF CACHE BOOL "Build c-ares as shared library" FORCE)
    set(CARES_BUILD_TOOLS OFF CACHE BOOL "Build c-ares tools" FORCE)
    add_subdirectory(c-ares)
    target_link_libraries(yasio c-ares)
    include_directories($<TARGET_FILE_DIR:c-ares>)
endif()

if (WIN32 AND YASIO_BUILD_TESTS)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "ftp_server")
endif ()
