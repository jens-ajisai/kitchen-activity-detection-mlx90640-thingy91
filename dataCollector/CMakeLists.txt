cmake_minimum_required(VERSION 3.20.0)

# We are only building for one board
set(BOARD nrf52840dongle_nrf52840)

# If CONF_FILE is set then boards/${BOARD}.conf is not automatically merged anymore!
# Therefore only use OVERLAY_CONFIG
list(APPEND OVERLAY_CONFIG "overlay-debug.conf")
list(APPEND OVERLAY_CONFIG "overlay-ble.conf")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(heaty_dataCollector)

target_sources(app PRIVATE
	src/main.c
	src/control.c
	src/shell_commands.c
	)
	
add_subdirectory(../modules build)

zephyr_include_directories(config)
zephyr_library_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-comment)