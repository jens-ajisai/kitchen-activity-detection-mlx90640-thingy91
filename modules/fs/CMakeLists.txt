target_sources_ifdef(CONFIG_FS_MODULE app PRIVATE
	fs_module.c
	fs_module_event.c
	fs_impl.c
	)