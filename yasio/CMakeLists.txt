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

if ( IOS )
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
endif ()

option(YAISO_BUILD_NI "Build with yasio-ni APIs support" OFF)
option(YASIO_BUILD_LUA "Build with lua bindings support" OFF)
option(YASIO_BUILD_CCLUA "Build with cocos lua support" OFF)
# only support for android
option(YASIO_BUILD_AS_SHARED "Build yasio as shared library" OFF) 

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
   xxsocket.hpp
   yasio.hpp
   ibstream.hpp
   obstream.hpp
   xxsocket.cpp
   yasio.cpp
   ibstream.cpp
   obstream.cpp
)

if (YAISO_BUILD_NI)
  set(YASIO_CORE ${YASIO_CORE} 
    bindings/yasio_ni.cpp
  )
endif()

if (YASIO_BUILD_LUA)
  set(YASIO_CORE ${YASIO_CORE} 
    bindings/lyasio.cpp
  )
endif()

if(YASIO_BUILD_CCLUA)
  set(YASIO_CORE ${YASIO_CORE} 
    bindings/yasio_cclua.cpp
  )
endif()

if(ANDROID AND CARES_INCLUDE_DIR)
  set(YASIO_CORE ${YASIO_CORE} 
    bindings/yasio_jni.cpp
  )
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

if (APPLE)
    if (IOS)
        set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
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
    if(ANDROID AND YASIO_BUILD_AS_SHARED)
        add_library(yasio SHARED
            ${YASIO_CORE}
            ${THIRDPART_SRC}
        )
        target_compile_definitions(yasio PUBLIC YASIO_BUILD_AS_SHARED=1)
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

# checking build system have openssl
if(OPENSSL_INCLUDE_DIR)
   target_compile_definitions(yasio PUBLIC YASIO_HAVE_SSL=1)
   target_include_directories(yasio PRIVATE "${OPENSSL_INCLUDE_DIR}")
endif()

# checking build system have c-ares
if(CARES_INCLUDE_DIR)
   target_compile_definitions(yasio PUBLIC YASIO_HAVE_CARES=1 YASIO_ENABLE_ARES_PROFILER=1)
   target_include_directories(yasio PRIVATE "${CARES_INCLUDE_DIR}")
   target_link_libraries(yasio c-ares)
endif()
