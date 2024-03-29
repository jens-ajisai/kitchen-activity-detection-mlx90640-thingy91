cmake_minimum_required(VERSION 3.20.0)

# We are only building for one board
set(BOARD thingy91_nrf9160_ns)

# If CONF_FILE is set then boards/${BOARD}.conf is not automatically merged anymore!
# Therefore only use OVERLAY_CONFIG
list(APPEND OVERLAY_CONFIG "overlay-credentials.conf")
list(APPEND OVERLAY_CONFIG "overlay-memfault.conf")
list(APPEND OVERLAY_CONFIG "overlay-watchdog.conf")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(heaty_nrf9160)

target_sources(app PRIVATE
	src/main.c
	src/control.c
	src/shell_commands.c
	)

target_sources_ifdef(CONFIG_SENSORS91_MODULE app PRIVATE
	src/sensors/sensors_module.c
	src/sensors/sensors_module_event.c
	)
	
target_sources_ifdef(CONFIG_ANALYSIS_MODULE app PRIVATE
	src/analysis/analysis_module.c
	src/analysis/analysis_module_event.c
	)
	
	
target_sources_ifdef(CONFIG_NRF_MODEM_LIB app PRIVATE
	src/https_client.c
	)

add_subdirectory_ifdef(CONFIG_USB_NRF9160_BRIDGE_AVAILABLE src/commCtrl/bridge/events)
add_subdirectory_ifdef(CONFIG_USB_NRF9160_BRIDGE_AVAILABLE src/commCtrl/bridge/modules)

target_sources_ifdef(CONFIG_COMMCTRL_MODULE app PRIVATE
	src/commCtrl/commCtrl_module.c
	src/commCtrl/commCtrl_module_event.c
	)

zephyr_library_include_directories(src/commCtrl/bridge/events)

add_subdirectory(../modules build)

zephyr_include_directories(../common)
zephyr_include_directories(config)
zephyr_include_directories(config/${BOARD})
zephyr_include_directories(src/sensors)
zephyr_include_directories(/opt/nordic/ncs/v1.8.0/nrfxlib/nrf_modem/include)
zephyr_include_directories(../nrf52840/src)
zephyr_include_directories(src)

# for some reason this is only applied if written in this top level CMakeLists.txt
# Did not work with the CMakeLists.txt in the lodepng module!
add_compile_definitions(
#	LODEPNG_NO_COMPILE_ZLIB 
#	LODEPNG_NO_COMPILE_DECODER 
	LODEPNG_NO_COMPILE_DISK 
	LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS 
	LODEPNG_NO_COMPILE_ERROR_TEXT 
	LODEPNG_NO_COMPILE_ALLOCATORS
)

zephyr_library_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-comment)
