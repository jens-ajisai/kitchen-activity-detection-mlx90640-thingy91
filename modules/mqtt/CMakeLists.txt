target_sources_ifdef(CONFIG_MQTT_MODULE app PRIVATE
	mqtt_client.c
	mqtt_module.c
	mqtt_module_event.c
	)