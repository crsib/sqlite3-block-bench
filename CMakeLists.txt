cmake_minimum_required(VERSION 3.19)

project(SQliteBlockSize)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
            TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

set(CMAKE_CXX_STANDARD 17)

conan_cmake_configure(
        REQUIRES
            fmt/7.1.3
            sqlite3/3.35.5
            lz4/1.9.3
            snappy/1.1.8
            gflags/2.2.2
        GENERATORS cmake_find_package)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
        BUILD missing
        REMOTE conan-center
        SETTINGS ${settings})

find_package(fmt REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(lz4 REQUIRED)
find_package(Snappy REQUIRED)
find_package(gflags REQUIRED)

add_executable(SQliteBlockSize main.cpp)
target_link_libraries(
    SQliteBlockSize
        fmt::fmt
        SQLite::SQLite3
        lz4::lz4
        Snappy::snappy
        gflags::gflags
)