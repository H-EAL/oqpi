cmake_minimum_required(VERSION 3.0)

project(oqpi)

find_package(Threads REQUIRED)
find_library(LIBRT rt REQUIRED)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
#Set debug mode using _GLIBCXX_DEBUG macro
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "..." FORCE)
endif()

##############################oqpi_tests##############################
add_executable(oqpi_tests tests/oqpi_tests.cpp tests/catch_amalgamated.cpp)

target_include_directories(oqpi_tests PRIVATE include)

# Link with lrt
target_link_libraries(oqpi_tests PRIVATE ${LIBRT})

# Link with pthread
set(THREADS_PREFER_FLAG ON)
target_link_libraries(oqpi_tests PRIVATE Threads::Threads)

install(TARGETS oqpi_tests DESTINATION bin)
