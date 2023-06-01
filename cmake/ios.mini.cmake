# See: https://github.com/yasio/ios.mini.cmake
# v4.0.0

if(NOT DEFINED CMAKE_SYSTEM_NAME)
    set(CMAKE_SYSTEM_NAME "iOS" CACHE STRING "The CMake system name for iOS")
endif()

# The best solution for fix try_compile failed with code sign currently
# since cmake-3.18.2, not required
# everyting for cmake toolchain config before project(xxx) is better
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    ${CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}
    "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED"
    "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)

# set(CMAKE_BUILD_WITH_INSTALL_RPATH YES)
# set(CMAKE_INSTALL_RPATH "@executable_path/Frameworks")
# set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks" ${CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS})

# Fix compile failed with armv7 deployment target >= 11.0, xcode clang will report follow error
# clang: error: invalid iOS deployment version '--target=armv7-apple-ios13.6', 
#        iOS 10 is the maximum deployment target for 32-bit targets
# If not defined CMAKE_OSX_DEPLOYMENT_TARGET, cmake will choose latest deployment target
if(NOT DEFINED DEPLOYMENT_TARGET)
    if("${CMAKE_OSX_ARCHITECTURES}" MATCHES ".*armv7.*")
       set(DEPLOYMENT_TARGET "10.0")
    else()
       set(DEPLOYMENT_TARGET "11.0")
    endif()
endif()
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    message(STATUS "The CMAKE_OSX_DEPLOYMENT_TARGET not defined, sets iOS minimum deployment target to ${DEPLOYMENT_TARGET}")
    set(CMAKE_OSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET} CACHE STRING "")
endif()
if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET)
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET} CACHE STRING "")
endif()

# Regards x86_64 as simulator
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64"  CACHE STRING "")
endif()
if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64")
    if (CMAKE_SYSTEM_NAME EQUAL "iOS")
        set(CMAKE_OSX_SYSROOT "iphonesimulator" CACHE STRING "")
    elseif(CMAKE_SYSTEM_NAME EQUAL "tvOS")
        set(CMAKE_OSX_SYSROOT "appletvsimulator" CACHE STRING "")
    elseif(CMAKE_SYSTEM_NAME EQUAL "watchsimulator")
        set(CMAKE_OSX_SYSROOT "watchsimulator" CACHE STRING "")
    else()
        message(FATAL_ERROR "CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} unsupported!")
    endif()
endif() 

# ENABLE_BITCODE, default ON
if(NOT DEFINED ENABLE_BITCODE
    set(ENABLE_BITCODE TRUE)
endif()
if(ENABLE_BITCODE)
    set(CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE "bitcode")
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "YES")
else()
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

# Sets CMAKE_SYSTEM_PROCESSOR for device and simulator properly
string(TOLOWER "${CMAKE_OSX_SYSROOT}" lowercase_CMAKE_OSX_SYSROOT)
if("${lowercase_CMAKE_OSX_SYSROOT}" MATCHES ".*simulator")
    set(IMC_IOS_PLAT "SIMULATOR" CACHE STRING "")
    if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "i386")
        set(CMAKE_SYSTEM_PROCESSOR i386)
    elseif("${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64")
        set(CMAKE_SYSTEM_PROCESSOR x86_64)
    else() # Since xcode12, default arch for simulator is arm64
        if(${XCODE_VERSION} LESS "12.0.0")
            set(CMAKE_SYSTEM_PROCESSOR x86_64)
        else()
            set(CMAKE_SYSTEM_PROCESSOR arm64)
        endif()
    endif()
else()
    set(IMC_IOS_PLAT "DEVICE" CACHE STRING "")
    set(CMAKE_SYSTEM_PROCESSOR arm64)
endif()

# This little function lets you set any XCode specific property, refer to: ios.toolchain.cmake
function(set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
    if (ARGC LESS 4 OR ARGV3 STREQUAL "All")
        set_property(TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY} ${XCODE_VALUE})
    else()
        set(XCODE_RELVERSION ${ARGV3})
        set_property(TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY}[variant=${XCODE_RELVERSION}] "${XCODE_VALUE}")
    endif()
endfunction()
