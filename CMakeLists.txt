cmake_minimum_required(VERSION 3.19)

project(SQliteBlockSize)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/develop/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

set(CMAKE_CXX_STANDARD 17)

set(CONAN_GENERATOR cmake_find_package_multi)

message(STATUS "Using conan generator ${CONAN_GENERATOR}")

conan_cmake_configure(
    REQUIRES
        fmt/7.1.3
        sqlite3/3.35.5
        lz4/1.9.3
        snappy/1.1.8
        benchmark/1.5.2
    GENERATORS ${CONAN_GENERATOR}
)

if(MSVC OR XCODE)
    foreach(TYPE ${CMAKE_CONFIGURATION_TYPES})
        message(STATUS "Configuring packages for ${TYPE}")
        
        conan_cmake_autodetect(settings BUILD_TYPE ${TYPE})

        conan_cmake_install(PATH_OR_REFERENCE .
            BUILD missing
            REMOTE conan-center
            SETTINGS ${settings}
        )
    endforeach()
else()
    conan_cmake_autodetect(settings)

    conan_cmake_install(PATH_OR_REFERENCE .
        BUILD missing
        REMOTE conan-center
        SETTINGS ${settings}
    )
endif()

find_package(fmt CONFIG)
find_package(SQLite3 CONFIG)
find_package(lz4 CONFIG)
find_package(Snappy CONFIG)
find_package(benchmark CONFIG)

add_executable(SQliteBlockSize 
    main.cpp
    
    Block.h
    Block.cpp

    DBHandle.h
    DBHandle.cpp

    DBStatement.h
    DBStatement.cpp

    DataBuffer.h
    DataBuffer.cpp
)

if(WIN32)
    target_compile_definitions(SQliteBlockSize PRIVATE -D_USE_MATH_DEFINES)
endif()

target_link_libraries(
    SQliteBlockSize
        fmt::fmt
        SQLite::SQLite3
        lz4::lz4
        Snappy::snappy
        benchmark::benchmark
)