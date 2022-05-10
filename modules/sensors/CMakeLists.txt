target_sources_ifdef(CONFIG_SENSORS_MODULE app PRIVATE
	sensors_module.c
	sensors_module_event.c
	)
	
target_sources_ifdef(CONFIG_SENSORS_MLX90640 app PRIVATE
	sensor_mlx90640.cpp
	MLX90640_API.cpp
	MLX90640_I2C_Driver.cpp
	)

