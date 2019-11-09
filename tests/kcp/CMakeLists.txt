set (KCPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (KCPTEST_SRC ${KCPTEST_SRC_DIR}/main.cpp ${KCPTEST_SRC_DIR}/../../yasio/kcp/ikcp.c)

set (KCPTEST_INC_DIR ${KCPTEST_SRC_DIR}/../../)

include_directories ("${KCPTEST_SRC_DIR}")
include_directories ("${KCPTEST_INC_DIR}")

add_executable (kcptest ${KCPTEST_SRC}) 

if (WIN32)
    set (KCPTEST_LDLIBS yasio)
else ()
    set (KCPTEST_LDLIBS yasio pthread)
endif()

# target_link_libraries (kcptest ${KCPTEST_LDLIBS})
