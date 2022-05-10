#ifndef _FS_IMPL_H_
#define _FS_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

int fs_module_setup();
int fs_helper_file_read(const char *file_path, char *file_contents, size_t *contents_len);
int fs_helper_file_write(const char *file_path, char const *file_contents, size_t contents_len, bool isAppend);
int fs_helper_file_delete(const char *file_path);

#ifdef __cplusplus
}
#endif

#endif
