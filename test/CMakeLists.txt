cmake_minimum_required(VERSION 3.0)

# Collect files and put them in source group folders
file(GLOB_RECURSE files *)
foreach(file ${files})
    file(RELATIVE_PATH relFile ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    get_filename_component(folder ${relFile} PATH)
    string(REPLACE / \\ folder "${folder}")
    source_group("${folder}" FILES ${file})
endforeach()

# Add executable
include_directories(../include)
add_executable(oqpi_tests ${files})
