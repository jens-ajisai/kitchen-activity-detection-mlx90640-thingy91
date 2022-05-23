cmake_minimum_required(VERSION 3.20.0)

# We are only building for one board
set(BOARD thingy91_nrf52840)

# If CONF_FILE is set then boards/${BOARD}.conf is not automatically merged anymore!
# Therefore only use OVERLAY_CONFIG
list(APPEND OVERLAY_CONFIG "overlay-credentials.conf")
list(APPEND OVERLAY_CONFIG "overlay-debug.conf")
list(APPEND OVERLAY_CONFIG "overlay-ble.conf")
list(APPEND OVERLAY_CONFIG "overlay-dfu.conf")
list(APPEND OVERLAY_CONFIG "overlay-memfault.conf")
list(APPEND OVERLAY_CONFIG "overlay-watchdog.conf")

#set(mcuboot_DTC_OVERLAY_FILE
# ${CMAKE_CURRENT_SOURCE_DIR}/child_image/mcuboot_dtc.overlay
#)

set(mcuboot_CONF_FILE
 ${CMAKE_CURRENT_SOURCE_DIR}/child_image/mcuboot.conf
)


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(heaty_nrf52840)

target_sources(app PRIVATE
	src/main.c
	src/shell_commands.c
	)

target_sources_ifdef(CONFIG_ANALYSIS_MODULE app PRIVATE
	src/analysis/analysis_module.c
	src/analysis/analysis_module_event.c
	)

target_sources_ifdef(CONFIG_MQTT_MODULE app PRIVATE
	src/mqtt/mqtt_client.c
	src/mqtt/mqtt_module.c
	src/mqtt/mqtt_module_event.c
	)

target_sources_ifdef(CONFIG_NRF_MODEM_LIB app PRIVATE
	src/https_client.c
	)


target_sources_ifdef(CONFIG_CAF_BUTTONS app PRIVATE
	src/button_command.c
	)

add_subdirectory(../modules build)

add_subdirectory_ifdef(CONFIG_USB_NRF9160_BRIDGE_AVAILABLE src/commCtrl/bridge/events)
add_subdirectory_ifdef(CONFIG_USB_NRF9160_BRIDGE_AVAILABLE src/commCtrl/bridge/modules)

target_sources_ifdef(CONFIG_COMMCTRL_MODULE app PRIVATE
	src/commCtrl/commCtrl_module.c
	src/commCtrl/commCtrl_module_event.c
	)

zephyr_library_include_directories(src/commCtrl/bridge/events)

zephyr_include_directories(config)
zephyr_include_directories(config/${BOARD})
zephyr_include_directories(/opt/nordic/ncs/v1.8.0/nrfxlib/nrf_modem/include)
zephyr_include_directories(src)

zephyr_library_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-comment)