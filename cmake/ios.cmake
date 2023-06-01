#
# The simple ios toolchain file: https://github.com/yasio/ios.cmake
# version: 4.0.2
#
# The supported params:
#   PLAT: iOS, tvOS, default: iOS
#   ARCHS: arm64, x86_64, default: arm64
#   DEPLOYMENT_TARGET: 11.0(default)
#   SIMULATOR: TRUE, FALSE, UNDEFINED(auto determine by archs) 
#   BITCODE: TRUE(default)
#

# PLAT
if(NOT DEFINED PLAT)
    set(PLAT iOS CACHE STRING "" FORCE)
endif()

# ARCHS
if(NOT DEFINED ARCHS)
    set(ARCHS "arm64" CACHE STRING "" FORCE)
endif()

# DEPLOYMENT_TARGET
if(NOT DEFINED DEPLOYMENT_TARGET)
    if("${ARCHS}" MATCHES ".*armv7.*")
       set(DEPLOYMENT_TARGET "10.0" CACHE STRING "" FORCE)
    else()
       set(DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
    endif()
endif()

# SIMULATOR, regards x86_64 as simulator if SIMULATOR not defined
if((NOT DEFINED SIMULATOR) AND ("${ARCHS}" STREQUAL "x86_64"))
    set(SIMULATOR TRUE CACHE BOOL "" FORCE)
endif()

# BITCODE, default ON
if(NOT DEFINED BITCODE)
    set(BITCODE TRUE CACHE BOOL "" FORCE)
endif()

# apply params
set(CMAKE_SYSTEM_NAME ${PLAT} CACHE STRING "")
set(CMAKE_OSX_ARCHITECTURES ${ARCHS} CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET} CACHE STRING "")

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
if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET)
    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET} CACHE STRING "")
endif()

if(SIMULATOR)
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CMAKE_OSX_SYSROOT "iphonesimulator" CACHE STRING "")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
        set(CMAKE_OSX_SYSROOT "appletvsimulator" CACHE STRING "")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "watchsimulator")
        set(CMAKE_OSX_SYSROOT "watchsimulator" CACHE STRING "")
    else()
        message(FATAL_ERROR "CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} unsupported!")
    endif()
endif() 

if(BITCODE)
    set(CMAKE_XCODE_ATTRIBUTE_BITCODE_GENERATION_MODE "bitcode")
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "YES")
else()
    set(CMAKE_XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
endif()

# Sets CMAKE_SYSTEM_PROCESSOR for device and simulator properly
string(TOLOWER "${CMAKE_OSX_SYSROOT}" lowercase_CMAKE_OSX_SYSROOT)
if("${lowercase_CMAKE_OSX_SYSROOT}" MATCHES ".*simulator")
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
