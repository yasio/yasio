set (KCPTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (KCPTEST_SRC ${KCPTEST_SRC_DIR}/main.cpp ${KCPTEST_SRC_DIR}/../../yasio/kcp/ikcp.c)

set (KCPTEST_INC_DIR ${KCPTEST_SRC_DIR}/../../)

include_directories ("${KCPTEST_SRC_DIR}")
include_directories ("${KCPTEST_INC_DIR}")

add_executable (kcptest ${KCPTEST_SRC}) 

if (NOT WIN32)
    set (KCPTEST_LDLIBS pthread)
    target_link_libraries (kcptest ${KCPTEST_LDLIBS})
endif()
