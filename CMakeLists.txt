cmake_minimum_required(VERSION 3.16)
project(AmcacheParser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Vendored dependencies
add_subdirectory(third_party/cli11 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/fmt EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)

set(AMCACHE_SOURCES
    src/registry/HiveParser.cpp
    src/amcache/AmcacheNew.cpp
    src/amcache/AmcacheOld.cpp
    src/amcache/Helper.cpp
    src/csv/CsvWriter.cpp
    src/utils/DateTimeUtils.cpp
    src/utils/RawCopy.cpp
)

set(AMCACHE_HEADERS
    src/registry/HiveParser.h
    src/registry/RegistryWrapper.h
    src/amcache/AmcacheNew.h
    src/amcache/AmcacheOld.h
    src/amcache/Helper.h
    src/classes/Common.h
    src/classes/FileEntryNew.h
    src/classes/ProgramsEntryNew.h
    src/classes/FileEntryOld.h
    src/classes/ProgramsEntryOld.h
    src/classes/Shortcut.h
    src/classes/DeviceContainer.h
    src/classes/DevicePnp.h
    src/classes/DriverBinary.h
    src/classes/DriverPackage.h
    src/csv/CsvWriter.h
    src/utils/DateTimeUtils.h
    src/utils/RawCopy.h
)

add_library(amcache STATIC ${AMCACHE_SOURCES} ${AMCACHE_HEADERS})

target_include_directories(amcache
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(amcache
    PUBLIC
        spdlog::spdlog
        fmt::fmt
)

if(WIN32)
    target_compile_definitions(amcache PRIVATE _WIN32_WINNT=0x0601 NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(amcache PRIVATE advapi32 kernel32)
endif()

if(MSVC)
    target_compile_options(amcache PRIVATE /W4 /WX-)
else()
    target_compile_options(amcache PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(AmcacheParser src/main.cpp)

target_link_libraries(AmcacheParser PRIVATE amcache CLI11::CLI11)

if(MSVC)
    target_compile_options(AmcacheParser PRIVATE /W4 /WX-)
else()
    target_compile_options(AmcacheParser PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)
install(TARGETS AmcacheParser RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
