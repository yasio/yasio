set (SSLTEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set (SSLTEST_SRC ${SSLTEST_SRC_DIR}/main.cpp)

set (SSLTEST_INC_DIR ${SSLTEST_SRC_DIR}/../../)

include_directories ("${SSLTEST_SRC_DIR}")
include_directories ("${SSLTEST_INC_DIR}")

add_executable (ssltest ${SSLTEST_SRC}) 

if (MSVC)
    set (SSLTEST_LDLIBS yasio ext_crypto ext_ssl)
else ()
    set (SSLTEST_LDLIBS yasio pthread ext_crypto ext_ssl dl)
endif()

target_link_libraries (ssltest ${SSLTEST_LDLIBS})

if (MSVC)
    add_custom_command(TARGET ssltest
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENSSL_LIB_DIR}/libcrypto-1_1.dll
        ${OPENSSL_LIB_DIR}/libssl-1_1.dll
        $<TARGET_FILE_DIR:ssltest>
        )
endif()