#include <zephyr.h>
#include <device.h>
#include <storage/disk_access.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>
#include <stdio.h>

#include "fs_impl.h"

LOG_MODULE_REGISTER(fs_impl, CONFIG_FS_MODULE_LOG_LEVEL);

static FATFS fat_fs;

static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
    .mnt_point = "/SD:"
};


static int test_sd_card_size()
{
    static const char *disk_pdrv = "SD";
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;
    int res;    

    res = disk_access_init(disk_pdrv);
    if (res != 0) {
        LOG_ERR("Storage init ERROR!");
        return res;
    }

    res = disk_access_ioctl(disk_pdrv,
            DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
    if (res) {
        LOG_ERR("Unable to get sector count");
        return res;
    }

    LOG_INF("Block count %u", block_count);

    res = disk_access_ioctl(disk_pdrv,
            DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
    if (res) {
        LOG_ERR("Unable to get sector size");
        return res;
    }
    LOG_INF("Sector size %u\n", block_size);

    memory_size_mb = (uint64_t)block_count * block_size;
    LOG_INF("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

    return 0;
}


int fs_helper_file_read(
    const char *file_path,
    char *file_contents,
    size_t *contents_len)
{
    struct fs_file_t file;
    ssize_t bytes_read;
    char fname[CONFIG_MAX_PATH_LENGTH + strlen(mp.mnt_point) + 1];
    int res;

    res = snprintf(fname, sizeof(fname), "%s/%s", mp.mnt_point, file_path);

#pragma GCC diagnostic ignored "-Wsign-compare" 
    if (res <= 0 || res > sizeof(fname)) {
        LOG_ERR("error with snprinntf");
        return -ENOMEM;
    }
#pragma GCC diagnostic pop      

    LOG_INF("fs_helper_file_read %s", fname);

    fs_file_t_init(&file);
    res = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
    if (res) {
        LOG_ERR("error fs_open");
        return res;
    }

    bytes_read = fs_read(&file, file_contents, *contents_len);
    LOG_INF("bytes_read: %d", bytes_read);
    *contents_len = bytes_read;

    res = fs_close(&file);
    if (res) {
        LOG_ERR("error fs_close");
        return res;
    }

    return 0;
}


int fs_helper_file_delete(const char *file_path)
{
    char fname[CONFIG_MAX_PATH_LENGTH + strlen(mp.mnt_point) + 1];
    int res;

    res = snprintf(fname, sizeof(fname), "%s/%s", mp.mnt_point, file_path);
#pragma GCC diagnostic ignored "-Wsign-compare" 
    if (res <= 0 || res > sizeof(fname)) {
    LOG_ERR("error with snprinntf");
       return -ENOMEM;
    }
#pragma GCC diagnostic pop    

    LOG_INF("fs_helper_file_delete %s", log_strdup(fname));

    res = fs_unlink(fname);
    if (res) {
        LOG_ERR("error fs_unlink");
        return res;
    }

    return 0;
}

int fs_helper_file_write(
    const char *file_path,
    char const *file_contents,
    size_t contents_len,
    bool isAppend)
{
    struct fs_file_t file;
    ssize_t bytes_written;
    char fname[CONFIG_MAX_PATH_LENGTH + strlen(mp.mnt_point) + 1];
    int res;

    res = snprintf(fname, sizeof(fname), "%s/%s", mp.mnt_point, file_path);
#pragma GCC diagnostic ignored "-Wsign-compare" 
    if (res <= 0 || res > sizeof(fname)) {
        LOG_ERR("error with snprinntf");
       return -ENOMEM;
    }
#pragma GCC diagnostic pop    

    LOG_INF("fs_helper_file_write %s", log_strdup(fname));


    fs_file_t_init(&file);

    if(isAppend)
    {
        res = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR | FS_O_APPEND);
    }
    else
    {
        // unlink not required, gives an error if failed
        // res = fs_unlink(fname);
        res = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
    }

    if (res) {
        LOG_ERR("error fs_open");
        return res;
    }

    bytes_written = fs_write(&file, file_contents, contents_len);
#pragma GCC diagnostic ignored "-Wsign-compare" 
    if (bytes_written != contents_len) {
        LOG_ERR("error fs_write");
        return -ENOMEM;
    }
#pragma GCC diagnostic pop    

    res = fs_close(&file);
    if (res) {
        LOG_ERR("error fs_close");
        return res;
    }

    return 0;
}

#pragma GCC diagnostic ignored "-Wunused-function" 
static int lsdir(const char *path)
{
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    fs_dir_t_init(&dirp);

    res = fs_opendir(&dirp, path);
    if (res) {
        LOG_ERR("Error opening dir %s [%d]", path, res);
        return res;
    }

    LOG_DBG("Listing dir %s ...", path);
    for (;;) {
        // Verify fs_readdir()
        res = fs_readdir(&dirp, &entry);

        // entry.name[0] == 0 means end-of-dir
        if (res || entry.name[0] == 0) {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            LOG_DBG("[DIR ] %s", entry.name);
        } else {
            LOG_DBG("[FILE] %s (size = %zu)",
                entry.name, entry.size);
        }
    }

    res = fs_closedir(&dirp);
    if (res) {
        LOG_ERR("error fs_closedir");
        return res;
    }

    return res;
}
#pragma GCC diagnostic pop    


int fs_module_setup()
{
    int res;
    res = test_sd_card_size();
    LOG_INF("test_sd_card_size, ret=%d", res);
    if(res) {
        return res;
    }
    
    // try an unmount in case of re-init
    fs_unmount(&mp);

    res = fs_mount(&mp);
    LOG_INF("fs_mount, ret=%d", res);
    if (res == FR_OK) {
        LOG_INF("Disk mounted.");
    } else {
        LOG_ERR("Error mounting disk.");
        return res;
    }

    res = lsdir("/");
    return res;
}
