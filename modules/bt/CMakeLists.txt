target_sources_ifdef(CONFIG_BLE_MODULE app PRIVATE
	bt_module.c
	bt_module_event.c
	)

target_sources_ifdef(CONFIG_SENSORS_MLX90640 app PRIVATE
	heaty_service.c
	)

target_sources_ifdef(CONFIG_BLE_ENABLE_CAMERA_SERVICE app PRIVATE
	camera_service.c
	)

target_sources_ifdef(CONFIG_BLE_NRF9160_BRIDGE_AVAILABLE app PRIVATE
	serial_nrf91_bridge_service.c
	)

target_sources_ifdef(CONFIG_BLE_ENABLE_TIME_SERVICE app PRIVATE
	time_service.c
	)
	