cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_UNIX)
    set(CMAKE_SYSTEM_NAME Windows)
    set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

    # cross compilers to use for C, C++ and RC
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
    set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

    # modify default behavior of FIND_XXX() commands
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(PKG_CONFIG_EXECUTABLE ${TOOLCHAIN_PREFIX}-pkg-config)
endif()

set(MY_PROJECT_NAME Mokthyn)
set(MY_TARGET_NAME Mokthyn)
set(MY_OUTPUT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/build")

project(${MY_PROJECT_NAME} LANGUAGES CXX)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration" FORCE)
endif()

file(GLOB DEMO_SOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

message("DEMO_SOURCE: ${DEMO_SOURCE}")

add_executable(${MY_TARGET_NAME} ${DEMO_SOURCE})
set_target_properties(${MY_TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${MY_OUTPUT_ROOT}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${MY_OUTPUT_ROOT}/Release"
)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/image")
    add_custom_target(copy-runtime-assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/image"
            "${MY_OUTPUT_ROOT}/$<CONFIG>/image"
        VERBATIM
    )
    add_dependencies(${MY_TARGET_NAME} copy-runtime-assets)
else()
    message(STATUS "Runtime asset directory not found; skipping asset copy")
endif()

target_include_directories(${MY_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ege/include)
target_compile_features(${MY_TARGET_NAME} PRIVATE cxx_std_17)
set_target_properties(${MY_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(${MY_TARGET_NAME} PRIVATE ws2_32 shell32 crypt32 bcrypt)

# 设置静态库搜索路径
if(MSVC)
    if(MSVC_VERSION GREATER_EQUAL 1950)
        # vs2026 以上, 静态库是兼容的.
        if(CMAKE_CL_64)
            set(osLibDir "vs2026/x64")
        else()
            set(osLibDir "vs2026/x86")
        endif()
    elseif(MSVC_VERSION GREATER_EQUAL 1930)
        # vs2022 以上, 静态库是兼容的.
        if(CMAKE_CL_64)
            set(osLibDir "vs2022/x64")
        else()
            set(osLibDir "vs2022/x86")
        endif()
    elseif(MSVC_VERSION GREATER_EQUAL 1920)
        # vs2019 以上, 静态库是兼容的.
        if(CMAKE_CL_64)
            set(osLibDir "vs2019/x64")
        else()
            set(osLibDir "vs2019/x86")
        endif()
    elseif(MSVC_VERSION GREATER_EQUAL 1910)
        # vs2017
        if(CMAKE_CL_64)
            set(osLibDir "vs2017/x64")
        else()
            set(osLibDir "vs2017/x86")
        endif()

    elseif(MSVC_VERSION GREATER_EQUAL 1900)
        # vs2015
        if(CMAKE_CL_64)
            set(osLibDir "vs2015/amd64")
        else()
            set(osLibDir "vs2015")
        endif()

    elseif(MSVC_VERSION GREATER_EQUAL 1600)
        if(MSVC_VERSION GREATER_EQUAL 1700)
            message(WARNING "You are using vs2012/vs2013, which is not tested, please use vs2010, vs2015 or later version of MSVC compiler.")
        endif()

        # vs2010
        if(CMAKE_CL_64)
            set(osLibDir "vs2010/amd64")
        else()
            set(osLibDir "vs2010")
        endif()

    else()
        message(FATAL_ERROR "你的 MSVC 版本太老了, 请使用 vs2015 或更新版本的 MSVC 编译器. 与时俱进吧, 不要一直使用十年前的编译器.")
    endif()

    target_compile_options(${MY_TARGET_NAME} PRIVATE
        /source-charset:utf-8
        /MP
        /D__STDC_LIMIT_MACROS
        /D_USE_MATH_DEFINES
        /Zc:__cplusplus
        "$<$<CONFIG:DEBUG>:/DDEBUG>"
        "$<$<CONFIG:RELEASE>:/DNDEBUG>"
        "$<$<CONFIG:RELWITHDEBINFO>:/DNDEBUG>"
        "$<$<CONFIG:MINSIZEREL>:/DNDEBUG>"
        "$<IF:$<CONFIG:Debug>,/MDd,/MD>")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(CMAKE_HOST_APPLE)
        set(osLibDir "macOS")
    elseif(CMAKE_HOST_UNIX)
        set(osLibDir "mingw-w64-debian")
    elseif(MINGW)
        set(osLibDir "mingw64")
    endif()

    target_compile_options(${MY_TARGET_NAME} PRIVATE -fstack-protector-strong)
    target_link_options(${MY_TARGET_NAME} PRIVATE -mwindows -static -static-libgcc -static-libstdc++)
    target_link_libraries(${MY_TARGET_NAME} PRIVATE graphics gdiplus gdi32 imm32 msimg32 ole32 oleaut32 winmm uuid)
endif()

if(DEFINED osLibDir)
    message(STATUS "osLibDir: ${osLibDir}")
    target_link_directories(${MY_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ege/lib/${osLibDir})
endif()
