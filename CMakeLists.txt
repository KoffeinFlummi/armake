cmake_minimum_required(VERSION 3.14)

project(armake_suite LANGUAGES C)

set(ARMAKE_VERSION "\"0.6.3\"")

find_package(FLEX REQUIRED)
flex_target(RapiyLexer src/rapify.l ${CMAKE_CURRENT_BINARY_DIR}/rapify.yy.c)

find_package(BISON REQUIRED)
bison_target(RapifyParser src/rapify.y ${CMAKE_CURRENT_BINARY_DIR}/rapify.tab.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/rapify.tab.h)

add_executable(armake
    src/binarize.c
    src/build.c
    src/derapify.c
    src/filesystem.c
    src/img2paa.c
    src/keygen.c
    src/main.c
    src/material.c
    src/matrix.c
    src/model_config.c
    src/p3d.c
    src/paa2img.c
    src/preprocess.c
    src/rapify.c
    src/sign.c
    src/unpack.c
    src/utils.c
    src/vector.c

    lib/minilzo.c
    lib/sha1.c

    ${CMAKE_CURRENT_BINARY_DIR}/rapify.yy.c
    ${CMAKE_CURRENT_BINARY_DIR}/rapify.tab.c)

target_include_directories(armake
    PRIVATE
        lib/ src/
        ${CMAKE_CURRENT_BINARY_DIR}
)

find_package(OpenSSL)
target_link_libraries(armake PRIVATE OpenSSL::Crypto)

if (UNIX)
    target_link_libraries(armake PRIVATE m)
endif()

target_compile_definitions(armake
    PRIVATE
        "VERSION=${ARMAKE_VERSION}"
)
