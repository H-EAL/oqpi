cmake_minimum_required(VERSION 3.0)
project(oqpi)

# Collect files and put them in source group folders
file(GLOB_RECURSE files include/*)
foreach(file ${files})
    file(RELATIVE_PATH relFile ${CMAKE_CURRENT_SOURCE_DIR} ${file})
    get_filename_component(folder ${relFile} PATH)
    string(REPLACE / \\ folder "${folder}")
    source_group("${folder}" FILES ${file})
endforeach()

# Add header-only library
add_custom_target(oqpi SOURCES ${files})

# Add oqpi_tests
add_subdirectory(test)
