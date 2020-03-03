set (SSLTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (SSLTEST_SRC ${SSLTEST_SRC_DIR}/main.cpp)

set (SSLTEST_INC_DIR ${SSLTEST_SRC_DIR}/../../)

include_directories ("${SSLTEST_SRC_DIR}")
include_directories ("${SSLTEST_INC_DIR}")

add_executable (ssltest ${SSLTEST_SRC}) 

if (WINDOWS)
    set (SSLTEST_LDLIBS yasio)
else ()
    set (SSLTEST_LDLIBS yasio pthread)
endif()

target_link_libraries (ssltest ${SSLTEST_LDLIBS})

# link ssl stubs
if (YASIO_BUILD_WITH_SSL)
    if (UNIX)
        target_link_libraries (ssltest ext_ssl ext_crypto dl)
    else ()
        target_link_libraries (ssltest ext_ssl ext_crypto)
    endif()

    if (WINDOWS)
        add_custom_command(TARGET ssltest
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENSSL_LIB_DIR}/libcrypto-1_1.dll
            ${OPENSSL_LIB_DIR}/libssl-1_1.dll
            $<TARGET_FILE_DIR:ssltest>
        )
    endif()
endif()
