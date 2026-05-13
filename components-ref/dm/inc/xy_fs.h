/**
 * @file xy_fs.h
 * @brief File System Abstraction Layer
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_FS_H
#define XY_FS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef XY_FS_MAX_DRIVES
#define XY_FS_MAX_DRIVES    4       // 最大驱动器数
#endif

#ifndef XY_FS_MAX_PATH
#define XY_FS_MAX_PATH      256     // 最大路径长度
#endif

/* ==================== 状态码 ==================== */

typedef enum {
    XY_FS_OK = 0,
    XY_FS_ERROR,
    XY_FS_INVALID_PARAM,
    XY_FS_NOT_FOUND,
    XY_FS_ALREADY_EXISTS,
    XY_FS_FULL,
    XY_FS_NOT_MOUNTED,
    XY_FS_NOT_SUPPORTED,
} xy_fs_status_t;

/* ==================== 文件模式 ==================== */

typedef enum {
    XY_FS_MODE_READ = 0x01,
    XY_FS_MODE_WRITE = 0x02,
    XY_FS_MODE_APPEND = 0x04,
    XY_FS_MODE_CREATE = 0x08,
    XY_FS_MODE_TRUNC = 0x10,
} xy_fs_mode_t;

/* ==================== 文件属性 ==================== */

typedef struct {
    char name[64];
    uint32_t size;
    uint8_t is_dir;
    uint32_t mtime;
} xy_fs_stat_t;

/* ==================== 文件句柄 ==================== */

typedef struct xy_fs_file {
    struct xy_fs_file *next;
    void *priv;                   // 私有数据
    const struct xy_fs *fs;       // 所属文件系统
    uint8_t mode;                 // 打开模式
    uint32_t pos;                 // 当前位置
} xy_fs_file_t;

/* ==================== 文件系统操作接口 ==================== */

typedef struct {
    // 基础操作
    int (*init)(void);
    int (*deinit)(void);
    int (*format)(void);
    
    // 文件操作
    int (*open)(xy_fs_file_t *file, const char *path, uint8_t mode);
    int (*close)(xy_fs_file_t *file);
    int (*read)(xy_fs_file_t *file, void *buf, size_t len);
    int (*write)(xy_fs_file_t *file, const void *buf, size_t len);
    int (*seek)(xy_fs_file_t *file, long offset, int whence);
    long (*tell)(xy_fs_file_t *file);
    
    // 文件属性
    int (*stat)(const char *path, xy_fs_stat_t *stat);
    int (*remove)(const char *path);
    int (*rename)(const char *old_path, const char *new_path);
    
    // 目录操作
    int (*opendir)(const char *path);
    int (*readdir)(char *name, size_t len);
    int (*closedir)(void);
} xy_fs_ops_t;

/* ==================== 文件系统结构 ==================== */

typedef struct xy_fs {
    const char *name;             // 文件系统名称
    const char *mount_point;      // 挂载点
    const xy_fs_ops_t *ops;       // 操作接口
    bool mounted;                 // 挂载状态
    void *priv;                   // 私有数据
} xy_fs_t;

/* ==================== API ==================== */

/**
 * @brief 注册文件系统
 */
int xy_fs_register(xy_fs_t *fs, const char *name, const xy_fs_ops_t *ops);

/**
 * @brief 挂载文件系统
 */
int xy_fs_mount(xy_fs_t *fs, const char *mount_point);

/**
 * @brief 卸载文件系统
 */
int xy_fs_unmount(xy_fs_t *fs);

/**
 * @brief 打开文件
 */
xy_fs_file_t* xy_fs_open(const char *path, uint8_t mode);

/**
 * @brief 关闭文件
 */
int xy_fs_close(xy_fs_file_t *file);

/**
 * @brief 读取文件
 */
int xy_fs_read(xy_fs_file_t *file, void *buf, size_t len);

/**
 * @brief 写入文件
 */
int xy_fs_write(xy_fs_file_t *file, const void *buf, size_t len);

/**
 * @brief 定位文件指针
 */
int xy_fs_seek(xy_fs_file_t *file, long offset, int whence);

/**
 * @brief 获取当前位置
 */
long xy_fs_tell(xy_fs_file_t *file);

/**
 * @brief 获取文件大小
 */
int xy_fs_size(const char *path, uint32_t *size);

/**
 * @brief 删除文件
 */
int xy_fs_remove(const char *path);

/**
 * @brief 重命名文件
 */
int xy_fs_rename(const char *old_path, const char *new_path);

/**
 * @brief 检查文件是否存在
 */
bool xy_fs_exists(const char *path);

/**
 * @brief 读取整个文件
 */
int xy_fs_read_file(const char *path, void *buf, size_t len, size_t *actual);

/**
 * @brief 写入整个文件
 */
int xy_fs_write_file(const char *path, const void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_FS_H */
