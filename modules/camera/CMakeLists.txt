target_sources_ifdef(CONFIG_CAMERA_MODULE app PRIVATE
	camera_module.c
	camera_module_event.c
	arducam_mini_2mp/ArduCAM.c
	arducam_mini_2mp/ArduCAM_i2c_common.c
	arducam_mini_2mp/ArduCAM_spi_common.c
	arducam_mini_2mp/ArducamMini2MP.c
	)
	
target_sources_ifdef(CONFIG_ARDUCAM_SPI_USE_NRFX app PRIVATE
	arducam_mini_2mp/ArduCAM_spi_nrfx.c
	)

target_sources_ifdef(CONFIG_ARDUCAM_SPI_USE_ZEPHYR app PRIVATE
	arducam_mini_2mp/ArduCAM_spi_zephyr.c
	)

target_sources_ifdef(CONFIG_ARDUCAM_TWI_USE_NRFX app PRIVATE
	arducam_mini_2mp/ArduCAM_i2c_nrfx.c
	)

target_sources_ifdef(CONFIG_ARDUCAM_TWI_USE_ZEPHYR app PRIVATE
	arducam_mini_2mp/ArduCAM_i2c_zephyr.c
	)