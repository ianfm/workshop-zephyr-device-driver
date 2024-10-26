set(MCP9808_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE PATH "MCP9808 module root directory")
add_subdirectory(drivers)
zephyr_include_directories(drivers)