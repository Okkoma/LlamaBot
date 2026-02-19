include(config_FindBLAS.cmake)

set(BLIS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3.0")
set(BLIS_BUILD_DIR  "${CMAKE_CURRENT_BINARY_DIR}")
set(BLIS_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(NOT EXISTS ${CMAKE_BINARY_DIR}/bin/libblis.so)

    message(STATUS "BLIS configure ...")
    if (ANDROID_ABI)

        if (${ANDROID_ABI} MATCHES v7)
            set(ARCHI arm32)
        elseif (${ANDROID_ABI} MATCHES v8)
            set(ARCHI arm64)
        else()
            set(ARCHI x86_64)
        endif()
        
        set(ANDROID_TARGET "aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}")
        set(CFLAGS "CFLAGS=--target=${ANDROID_TARGET}\ --sysroot=${CMAKE_SYSROOT}\ -fPIC")
        set(LDFLAGS "LDFLAGS=--target=${ANDROID_TARGET}\ --sysroot=${CMAKE_SYSROOT}\ -Wl,-soname,libblis.so")
        set(LIBPTHREAD "LIBPTHREAD=${CMAKE_SYSROOT}/usr/lib/aarch64-linux-android/${ANDROID_NATIVE_API_LEVEL}/libc++.so")

        message ("ANDROID_NDK=${ANDROID_NDK}")
        message ("CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
        message ("CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
        message ("CMAKE_AR=${CMAKE_AR}")
        message ("CMAKE_RANLIB=${CMAKE_RANLIB}")
        message ("${LIBPTHREAD}")

    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(ARCHI x86_64)
    else()  
        message(WARNING "Unsupported platform: ${CMAKE_SYSTEM_PROCESSOR}")
        set(ARCHI generic)
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env 
                CC=${CMAKE_C_COMPILER}
                CXX=${CMAKE_CXX_COMPILER}
                AR=${CMAKE_AR}
                RANLIB=${CMAKE_RANLIB}
                ${LIBPTHREAD}
                ${CFLAGS}
                ${LDFLAGS}
                ${BLIS_SOURCE_DIR}/configure
                    --prefix=${BLIS_INSTALL_DIR}
                    --enable-threading=pthreads
                    --enable-cblas
                    ${ARCHI}
        WORKING_DIRECTORY ${BLIS_BUILD_DIR}
        RESULT_VARIABLE   CONF_RESULT
    )

    if(NOT CONF_RESULT EQUAL 0)
        message(FATAL_ERROR "Échec de la configuration de BLIS")
    endif()

    cmake_host_system_information(RESULT N_CORES QUERY NUMBER_OF_LOGICAL_CORES)
    if(NOT N_CORES OR N_CORES EQUAL 0)
        set(N_CORES 2)
    endif()

    message(STATUS "Compilation de BLIS sur ${N_CORES} threads...")
    execute_process(
        COMMAND make -j${N_CORES} install
        WORKING_DIRECTORY ${BLIS_BUILD_DIR}
        RESULT_VARIABLE   BUILD_RESULT
    )

    if(NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Échec de la compilation de BLIS")
    endif()

    if(NOT EXISTS "${CMAKE_BINARY_DIR}/bin")
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()

    if (ANDROID)
        file(COPY_FILE ${BLIS_INSTALL_DIR}/lib/${ARCHI}/libblis.so ${CMAKE_BINARY_DIR}/bin/libblis.so)
    else()
        file(COPY ${BLIS_INSTALL_DIR}/lib/${ARCHI}/libblis.so.4 DESTINATION ${CMAKE_BINARY_DIR}/bin FOLLOW_SYMLINK_CHAIN)
    endif()

else()

    message(STATUS "BLIS déjà compilé, on passe l'étape.")

endif()

message (STATUS "adding imported library ${CMAKE_BINARY_DIR}/bin/libblis.so")

add_library(blis SHARED IMPORTED GLOBAL)
set_target_properties(blis PROPERTIES
    IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/bin/libblis.so
    INTERFACE_INCLUDE_DIRECTORIES ${BLIS_INSTALL_DIR}/include
)

install(FILES ${CMAKE_BINARY_DIR}/bin/libblis.so DESTINATION lib)