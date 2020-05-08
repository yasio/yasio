# A cross platform socket APIs, support ios & android & wp8 & window store universal app.
# Copyright (c) 2012-2020 HALX99.
# Licensed under the MIT License (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
# http://opensource.org/licenses/MIT
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

cmake_minimum_required(VERSION 3.10)

if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD /O2" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /Od /Zi" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Bv" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /Od /Zi /Bv" CACHE STRING "")
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
option(YASIO_BUILD_WITH_CXX17 "Build yasio with c++17 standard" ON)

MARK_AS_ADVANCED(YASIO_PROJECT_DIR)

if (NOT WIN32 OR CYGWIN)
    if (YASIO_BUILD_WITH_CXX17)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
        message("building yasio with c++17 support")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
        message("building yasio with c++11 support")
    endif()
endif()

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


######### The yasio optional dependencies

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
    set(CARES_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external" CACHE INTERNAL "c-ares Include Directory" )
endif()

set(YAISO_BUILD_NI ON CACHE BOOL "Build with yasio-ni APIs support" FORCE)
add_subdirectory(yasio)

if ( WIN32 AND NOT CYGWIN )
    target_compile_features(yasio PUBLIC cxx_std_17)
endif()

if(NOT IOS AND YASIO_BUILD_TESTS)
    add_subdirectory(tests/tcp)
    add_subdirectory(tests/mcast)
    add_subdirectory(tests/kcp)
    add_subdirectory(tests/issue166)
    add_subdirectory(tests/issue178)
    add_subdirectory(tests/issue201)
    add_subdirectory(tests/issue208)
    add_subdirectory(tests/issue245)
    add_subdirectory(tests/issue256)
    add_subdirectory(tests/echo_server)
    add_subdirectory(tests/echo_client)
    add_subdirectory(examples/lua)
    add_subdirectory(examples/ftp_server)

    if(YASIO_BUILD_WITH_SSL)
        add_subdirectory(tests/ssl)
    endif()
endif ()

### Sets startup project for vs .sln
# if (WIN32 AND YASIO_BUILD_TESTS)
#     set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "ftp_server")
# endif ()
