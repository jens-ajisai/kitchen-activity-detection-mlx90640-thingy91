zephyr_include_directories(.)

add_subdirectory_ifdef(CONFIG_BLE_MODULE bt)
add_subdirectory_ifdef(CONFIG_CAMERA_MODULE camera)
add_subdirectory_ifdef(CONFIG_FS_MODULE fs)
add_subdirectory_ifdef(CONFIG_SENSORS_MODULE sensors)
add_subdirectory_ifdef(CONFIG_MCU_EXCHANGE_MODULE mcu_exchange)
add_subdirectory_ifdef(CONFIG_WATCHDOG watchdog)
add_subdirectory_ifdef(CONFIG_LODEPNG lodepng)
add_subdirectory_ifdef(CONFIG_MQTT_MODULE mqtt)

target_sources_ifdef(CONFIG_USB_DEVICE_STACK app PRIVATE
	usb_console.c
	)

target_sources_ifdef(CONFIG_MEMFAULT app PRIVATE
	common/diag.c
	)

target_sources(app PRIVATE
	common/modules_common.c
	common/memory_hook.c
	common/memory_statistics.c
	utils.c	
	util_settings.c
	util_gpio.c
	)