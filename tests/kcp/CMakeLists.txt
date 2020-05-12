set (target_name kcptest)

set (KCPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (KCPTEST_SRC 
    ${KCPTEST_SRC_DIR}/main.cpp
)

set (KCPTEST_INC_DIR ${KCPTEST_SRC_DIR}/../../)

include_directories ("${KCPTEST_SRC_DIR}")
include_directories ("${KCPTEST_INC_DIR}")

add_executable (${target_name} ${KCPTEST_SRC}) 

if (WIN32)
    set (KCPTEST_LDLIBS yasio)
else ()
    set (KCPTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (${target_name} ${KCPTEST_LDLIBS})

ConfigTargetSSL(${target_name})
