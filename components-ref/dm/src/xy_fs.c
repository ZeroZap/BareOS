/**
 * @file xy_fs.c
 * @brief File System Abstraction Layer Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_fs.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#define XY_FS_MAX_DRIVES    4

static xy_fs_t *g_fs_drives[XY_FS_MAX_DRIVES] = {0};

/**
 * @brief 注册文件系统
 */
int xy_fs_register(xy_fs_t *fs, const char *name, const xy_fs_ops_t *ops)
{
    if (!fs || !name || !ops) {
        return XY_FS_INVALID_PARAM;
    }

    /* 查找空闲槽位 */
    for (int i = 0; i < XY_FS_MAX_DRIVES; i++) {
        if (g_fs_drives[i] == NULL) {
            fs->name = name;
            fs->ops = ops;
            fs->mounted = false;
            g_fs_drives[i] = fs;

            xy_log_i("FS registered: %s (slot=%d)\n", name, i);
            return XY_FS_OK;
        }
    }

    return XY_FS_FULL;
}

/**
 * @brief 挂载文件系统
 */
int xy_fs_mount(xy_fs_t *fs, const char *mount_point)
{
    if (!fs || !mount_point) {
        return XY_FS_INVALID_PARAM;
    }

    if (fs->mounted) {
        return XY_FS_ERROR;
    }

    fs->mount_point = mount_point;

    if (fs->ops->init) {
        int ret = fs->ops->init();
        if (ret != XY_FS_OK) {
            return ret;
        }
    }

    fs->mounted = true;
    xy_log_i("FS mounted: %s at %s\n", fs->name, mount_point);
    return XY_FS_OK;
}

/**
 * @brief 卸载文件系统
 */
int xy_fs_unmount(xy_fs_t *fs)
{
    if (!fs || !fs->mounted) {
        return XY_FS_INVALID_PARAM;
    }

    if (fs->ops->deinit) {
        fs->ops->deinit();
    }

    fs->mounted = false;
    xy_log_i("FS unmounted: %s\n", fs->name);
    return XY_FS_OK;
}

/**
 * @brief 解析路径，找到对应文件系统
 */
static xy_fs_t* xy_fs_find_drive(const char *path, const char **rel_path)
{
    if (!path || !rel_path) {
        return NULL;
    }

    /* 简化实现：假设路径格式为 "0:/path/to/file" */
    if (path[1] == ':') {
        int drive_num = path[0] - '0';
        if (drive_num >= 0 && drive_num < XY_FS_MAX_DRIVES &&
            g_fs_drives[drive_num] && g_fs_drives[drive_num]->mounted) {
            *rel_path = &path[2];  /* 跳过 "0:" */
            if (*rel_path[0] == '/') {
                (*rel_path)++;  /* 跳过前导 / */
            }
            return g_fs_drives[drive_num];
        }
    }

    return NULL;
}

/**
 * @brief 打开文件
 */
xy_fs_file_t* xy_fs_open(const char *path, uint8_t mode)
{
    if (!path) {
        return NULL;
    }

    const char *rel_path;
    xy_fs_t *fs = xy_fs_find_drive(path, &rel_path);
    if (!fs || !fs->mounted) {
        xy_log_e("FS not found or not mounted: %s\n", path);
        return NULL;
    }

    if (!fs->ops->open) {
        return NULL;
    }

    /* 分配文件句柄 */
    xy_fs_file_t *file = (xy_fs_file_t *)calloc(1, sizeof(xy_fs_file_t));
    if (!file) {
        return NULL;
    }

    file->fs = fs;
    file->mode = mode;
    file->pos = 0;

    /* 调用底层打开 */
    int ret = fs->ops->open(file, rel_path, mode);
    if (ret != XY_FS_OK) {
        free(file);
        return NULL;
    }

    xy_log_d("FS open: %s (mode=%d)\n", path, mode);
    return file;
}

/**
 * @brief 关闭文件
 */
int xy_fs_close(xy_fs_file_t *file)
{
    if (!file) {
        return XY_FS_INVALID_PARAM;
    }

    if (!file->fs || !file->fs->ops) {
        free(file);
        return XY_FS_INVALID_PARAM;
    }

    /* 调用底层 close 操作 */
    int ret = XY_FS_OK;
    if (file->fs->ops->close) {
        ret = file->fs->ops->close(file);
    }

    xy_log_d("FS close: ret=%d\n", ret);
    free(file);
    return ret;
}

/**
 * @brief 读取文件
 */
int xy_fs_read(xy_fs_file_t *file, void *buf, size_t len)
{
    if (!file || !buf || len == 0) {
        return XY_FS_INVALID_PARAM;
    }

    if (!file->fs->ops->read) {
        return XY_FS_NOT_SUPPORTED;
    }

    /* 调用底层 read 操作 */
    int ret = file->fs->ops->read(file, buf, len);
    
    if (ret > 0) {
        file->pos += ret;
    }
    
    xy_log_d("FS read: len=%d, ret=%d\n", len, ret);
    return ret;
}

/**
 * @brief 写入文件
 */
int xy_fs_write(xy_fs_file_t *file, const void *buf, size_t len)
{
    if (!file || !buf || len == 0) {
        return XY_FS_INVALID_PARAM;
    }

    if (!file->fs->ops->write) {
        return XY_FS_NOT_SUPPORTED;
    }

    /* 调用底层 write 操作 */
    int ret = file->fs->ops->write(file, buf, len);
    
    if (ret > 0) {
        file->pos += ret;
    }
    
    xy_log_d("FS write: len=%d, ret=%d\n", len, ret);
    return ret;
}

/**
 * @brief 定位文件指针
 */
int xy_fs_seek(xy_fs_file_t *file, long offset, int whence)
{
    if (!file) {
        return XY_FS_INVALID_PARAM;
    }

    if (file->fs->ops->seek) {
        int ret = file->fs->ops->seek(file, offset, whence);
        if (ret == XY_FS_OK) {
            file->pos = offset;  /* 简化实现 */
        }
        return ret;
    }

    /* 默认实现 */
    switch (whence) {
        case 0: /* SEEK_SET */
            file->pos = offset;
            break;
        case 1: /* SEEK_CUR */
            file->pos += offset;
            break;
        case 2: /* SEEK_END */
            /* 需要知道文件大小 */
            return XY_FS_NOT_SUPPORTED;
    }

    return XY_FS_OK;
}

/**
 * @brief 获取当前位置
 */
long xy_fs_tell(xy_fs_file_t *file)
{
    if (!file) {
        return -1;
    }

    if (file->fs->ops->tell) {
        return file->fs->ops->tell(file);
    }

    return file->pos;
}

/**
 * @brief 获取文件大小
 */
int xy_fs_size(const char *path, uint32_t *size)
{
    if (!path || !size) {
        return XY_FS_INVALID_PARAM;
    }

    const char *rel_path;
    xy_fs_t *fs = xy_fs_find_drive(path, &rel_path);
    if (!fs || !fs->mounted) {
        return XY_FS_NOT_MOUNTED;
    }

    if (!fs->ops->stat) {
        return XY_FS_NOT_SUPPORTED;
    }

    xy_fs_stat_t stat;
    int ret = fs->ops->stat(rel_path, &stat);
    if (ret == XY_FS_OK) {
        *size = stat.size;
    }

    return ret;
}

/**
 * @brief 删除文件
 */
int xy_fs_remove(const char *path)
{
    if (!path) {
        return XY_FS_INVALID_PARAM;
    }

    const char *rel_path;
    xy_fs_t *fs = xy_fs_find_drive(path, &rel_path);
    if (!fs || !fs->mounted) {
        return XY_FS_NOT_MOUNTED;
    }

    if (!fs->ops->remove) {
        return XY_FS_NOT_SUPPORTED;
    }

    return fs->ops->remove(rel_path);
}

/**
 * @brief 重命名文件
 */
int xy_fs_rename(const char *old_path, const char *new_path)
{
    if (!old_path || !new_path) {
        return XY_FS_INVALID_PARAM;
    }

    const char *old_rel, *new_rel;
    xy_fs_t *fs = xy_fs_find_drive(old_path, &old_rel);
    if (!fs || !fs->mounted) {
        return XY_FS_NOT_MOUNTED;
    }

    /* 检查是否同一文件系统 */
    xy_fs_find_drive(new_path, &new_rel);
    if (fs != xy_fs_find_drive(new_path, &new_rel)) {
        return XY_FS_NOT_SUPPORTED;
    }

    if (!fs->ops->rename) {
        return XY_FS_NOT_SUPPORTED;
    }

    return fs->ops->rename(old_rel, new_rel);
}

/**
 * @brief 检查文件是否存在
 */
bool xy_fs_exists(const char *path)
{
    if (!path) {
        return false;
    }

    uint32_t size;
    return xy_fs_size(path, &size) == XY_FS_OK;
}

/**
 * @brief 读取整个文件
 */
int xy_fs_read_file(const char *path, void *buf, size_t len, size_t *actual)
{
    xy_fs_file_t *file = xy_fs_open(path, XY_FS_MODE_READ);
    if (!file) {
        return XY_FS_ERROR;
    }

    int ret = xy_fs_read(file, buf, len);
    if (ret > 0 && actual) {
        *actual = ret;
    }

    xy_fs_close(file);
    return ret > 0 ? XY_FS_OK : XY_FS_ERROR;
}

/**
 * @brief 写入整个文件
 */
int xy_fs_write_file(const char *path, const void *buf, size_t len)
{
    xy_fs_file_t *file = xy_fs_open(path, XY_FS_MODE_WRITE | XY_FS_MODE_CREATE | XY_FS_MODE_TRUNC);
    if (!file) {
        return XY_FS_ERROR;
    }

    int ret = xy_fs_write(file, buf, len);
    xy_fs_close(file);

    return ret > 0 ? XY_FS_OK : XY_FS_ERROR;
}
