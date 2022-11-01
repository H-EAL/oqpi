cmake_minimum_required(VERSION 3.0)

# Collect files and put them in source group folders
file(GLOB_RECURSE files CONFIGURE_DEPENDS *)
foreach(file ${files})
    file(RELATIVE_PATH relFile ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    get_filename_component(folder ${relFile} PATH)
    string(REPLACE / \\ folder "${folder}")
    source_group("${folder}" FILES ${file})
endforeach()

# Add executable
include_directories(../include)
add_executable(oqpi_tests ${files})

#Set debug mode using _GLIBCXX_DEBUG macro
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")

# Link with lrt and pthread
set(CMAKE_EXE_LINKER_FLAGS "-lrt")

# Link with pthread
set(THREADS_PREFER_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)
TARGET_LINK_LIBRARIES(oqpi_tests PRIVATE Threads::Threads)

# Set cpp version
set_target_properties(oqpi_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
