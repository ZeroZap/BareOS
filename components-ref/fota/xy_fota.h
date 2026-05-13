/**
 * @file xy_fota.h
 * @brief Firmware Over-The-Air Update Framework
 * @version 1.1.0
 * @date 2026-04-02
 */

#ifndef XY_FOTA_H
#define XY_FOTA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_FOTA_MAX_IMAGE_SIZE
#define XY_FOTA_MAX_IMAGE_SIZE  (256 * 1024)  /* 256KB */
#endif

#ifndef XY_FOTA_SLOT_COUNT
#define XY_FOTA_SLOT_COUNT      2  /* Dual bank default */
#endif

/* ==================== Error Codes ==================== */

#define XY_FOTA_OK                  0
#define XY_FOTA_ERROR               (-1)
#define XY_FOTA_INVALID_PARAM       (-2)
#define XY_FOTA_FLASH_ERROR         (-3)
#define XY_FOTA_CRC_ERROR           (-4)
#define XY_FOTA_AUTH_ERROR          (-5)
#define XY_FOTA_NO_IMAGE            (-6)
#define XY_FOTA_IN_PROGRESS         (-7)
#define XY_FOTA_NO_MEM              (-8)
#define XY_FOTA_TIMEOUT             (-9)
#define XY_FOTA_VERSION_ERROR       (-10)  /* 版本过旧 */
#define XY_FOTA_NO_BACKUP           (-11)  /* 无备份 */
#define XY_FOTA_DELTA_ERROR         (-12)  /* 增量升级失败 */

/* ==================== FOTA Mode ==================== */

/**
 * @brief FOTA 模式
 */
typedef enum {
    XY_FOTA_MODE_DUAL_BANK = 0,   /* 双槽模式 */
    XY_FOTA_MODE_SINGLE_SLOT,     /* 单槽模式 (需要备份区) */
    XY_FOTA_MODE_DELTA,           /* 增量升级模式 */
} xy_fota_mode_t;

/**
 * @brief FOTA 状态
 */
typedef enum {
    XY_FOTA_STATE_IDLE = 0,
    XY_FOTA_STATE_DOWNLOADING,
    XY_FOTA_STATE_PATCHING,       /* 增量升级打补丁中 */
    XY_FOTA_STATE_VALIDATING,
    XY_FOTA_STATE_BACKUP,        /* 备份当前固件 */
    XY_FOTA_STATE_UPDATING,
    XY_FOTA_STATE_VERIFYING,
    XY_FOTA_STATE_ROLLBACK,      /* 回滚中 */
    XY_FOTA_STATE_COMPLETE,
    XY_FOTA_STATE_ERROR,
} xy_fota_state_t;

/**
 * @brief FOTA 固件头
 */
typedef struct {
    uint32_t magic;             /* 魔数 0x464F5441 */
    uint32_t version;           /* 固件版本 */
    uint32_t image_size;        /* 镜像大小 */
    uint32_t crc32;             /* CRC32 校验 */
    uint32_t timestamp;         /* 时间戳 */
    uint32_t min_version;       /* 最低兼容版本 (防回滚) */
    uint32_t delta_size;        /* 增量包大小 (0=全量) */
    uint32_t delta_crc32;       /* 增量包CRC */
    uint8_t  flags;             /* 标志位 */
    uint8_t  reserved[7];       /* 保留 */
} __attribute__((packed)) xy_fota_header_t;

/* 固件头标志 */
#define XY_FOTA_FLAG_ENCRYPTED     (1 << 0)  /* 加密固件 */
#define XY_FOTA_FLAG_DELTA        (1 << 1)  /* 增量固件 */
#define XY_FOTA_FLAG_COMPRESSED    (1 << 2)  /* 压缩固件 */

/**
 * @brief FOTA Flash 操作回调 (支持内部/外部 Flash)
 */
typedef struct {
    int (*init)(void);
    int (*write)(uint32_t addr, const uint8_t *data, uint32_t size);
    int (*read)(uint32_t addr, uint8_t *data, uint32_t size);
    int (*erase)(uint32_t addr, uint32_t size);
    int (*deinit)(void);
} xy_fota_flash_ops_t;

/**
 * @brief FOTA 配置
 */
typedef struct {
    xy_fota_mode_t mode;            /* FOTA 模式 */
    uint32_t flash_base_addr;       /* Flash 基地址 */
    uint32_t slot_size;             /* 槽位大小 */
    uint8_t  slot_count;            /* 槽位数量 (1=单槽, 2=双槽) */
    uint32_t backup_addr;           /* 备份区地址 (单槽模式必须) */
    uint32_t backup_size;           /* 备份区大小 */
    bool     enable_secure_boot;    /* 安全启动 */
    bool     enable_rollback;       /* 启用回滚功能 */
    uint32_t min_version;           /* 最低版本号 (防回滚) */
} xy_fota_config_t;

/**
 * @brief FOTA 进度回调
 */
typedef void (*xy_fota_progress_cb)(uint32_t current, uint32_t total, void *user_data);

/**
 * @brief 增量升级补丁回调 (前置声明)
 */
typedef int (*xy_fota_patch_cb)(uint32_t offset, const uint8_t *data, uint32_t size, void *user_data);

/**
 * @brief FOTA 句柄
 */
typedef struct {
    xy_fota_config_t config;
    xy_fota_state_t state;
    xy_fota_header_t header;
    uint32_t downloaded_bytes;
    uint32_t current_slot;
    uint32_t active_slot;           /* 当前运行槽位 */
    uint32_t backup_version;        /* 备份版本号 */
    xy_fota_progress_cb progress_cb;
    void *user_data;
    const xy_fota_flash_ops_t *flash_ops;
    const xy_fota_flash_ops_t *backup_flash_ops;  /* 备份区 Flash (可选) */
    bool initialized;
} xy_fota_t;

/* ==================== FOTA Operations ==================== */

/**
 * @brief 初始化 FOTA
 * @param fota FOTA 句柄
 * @param config 配置
 * @return XY_FOTA_OK 成功
 */
int xy_fota_init(xy_fota_t *fota, const xy_fota_config_t *config);

/**
 * @brief 反初始化 FOTA
 * @param fota FOTA 句柄
 * @return XY_FOTA_OK 成功
 */
int xy_fota_deinit(xy_fota_t *fota);

/**
 * @brief 开始固件下载
 * @param fota FOTA 句柄
 * @param version 固件版本
 * @param size 固件大小 (全量或增量包大小)
 * @param is_delta 是否为增量包
 * @return XY_FOTA_OK 成功
 */
int xy_fota_start_download(xy_fota_t *fota, uint32_t version, uint32_t size, bool is_delta);

/**
 * @brief 下载固件数据块
 * @param fota FOTA 句柄
 * @param data 数据块
 * @param size 数据块大小
 * @return XY_FOTA_OK 成功
 */
int xy_fota_download_chunk(xy_fota_t *fota, const uint8_t *data, uint32_t size);

/**
 * @brief 完成下载并验证
 * @param fota FOTA 句柄
 * @return XY_FOTA_OK 成功
 */
int xy_fota_finish_download(xy_fota_t *fota);

/**
 * @brief 开始固件更新
 * @param fota FOTA 句柄
 * @return XY_FOTA_OK 成功
 */
int xy_fota_start_update(xy_fota_t *fota);

/**
 * @brief 回滚到上一版本
 * @param fota FOTA 句柄
 * @return XY_FOTA_OK 成功
 */
int xy_fota_rollback(xy_fota_t *fota);

/**
 * @brief 检查是否需要回滚
 * @param fota FOTA 句柄
 * @return true 需要回滚
 */
bool xy_fota_needs_rollback(xy_fota_t *fota);

/**
 * @brief 获取当前运行固件版本
 * @param fota FOTA 句柄
 * @return 版本号
 */
uint32_t xy_fota_get_current_version(xy_fota_t *fota);

/**
 * @brief 获取 FOTA 状态
 * @param fota FOTA 句柄
 * @return 当前状态
 */
xy_fota_state_t xy_fota_get_state(xy_fota_t *fota);

/**
 * @brief 获取下载进度
 * @param fota FOTA 句柄
 * @return 进度百分比 (0-100)
 */
uint8_t xy_fota_get_progress(xy_fota_t *fota);

/**
 * @brief 设置进度回调
 * @param fota FOTA 句柄
 * @param cb 回调函数
 * @param user_data 用户数据
 * @return XY_FOTA_OK 成功
 */
int xy_fota_set_progress_callback(xy_fota_t *fota, xy_fota_progress_cb cb, void *user_data);

/**
 * @brief 取消更新
 * @param fota FOTA 句柄
 * @return XY_FOTA_OK 成功
 */
int xy_fota_cancel(xy_fota_t *fota);

/**
 * @brief 重置 FOTA
 * @param fota FOTA 句柄
 * @return XY_FOTA_OK 成功
 */
int xy_fota_reset(xy_fota_t *fota);

/* ==================== Flash Operations ==================== */

/**
 * @brief 设置 Flash 操作接口 (主 Flash)
 * @param fota FOTA 句柄
 * @param ops Flash 操作接口
 * @return XY_FOTA_OK 成功
 */
int xy_fota_set_flash_ops(xy_fota_t *fota, const xy_fota_flash_ops_t *ops);

/**
 * @brief 设置备份区 Flash 操作接口 (外部 Flash 可选)
 * @param fota FOTA 句柄
 * @param ops Flash 操作接口
 * @return XY_FOTA_OK 成功
 */
int xy_fota_set_backup_flash_ops(xy_fota_t *fota, const xy_fota_flash_ops_t *ops);

/* ==================== Helper Functions ==================== */

/**
 * @brief 计算 CRC32
 * @param data 数据
 * @param size 数据大小
 * @return CRC32 值
 */
uint32_t xy_fota_calc_crc32(const uint8_t *data, uint32_t size);

/**
 * @brief 验证固件头
 * @param header 固件头
 * @return true 有效
 */
bool xy_fota_validate_header(const xy_fota_header_t *header);

/**
 * @brief 验证版本号 (防回滚)
 * @param fota FOTA 句柄
 * @param version 要验证的版本
 * @return true 版本有效
 */
bool xy_fota_validate_version(xy_fota_t *fota, uint32_t version);

/* ==================== Delta Patching ==================== */

/**
 * @brief 设置增量补丁回调
 * @param fota FOTA 句柄
 * @param cb 补丁回调
 * @param user_data 用户数据
 * @return XY_FOTA_OK 成功
 */
int xy_fota_set_patch_callback(xy_fota_t *fota, xy_fota_patch_cb cb, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* XY_FOTA_H */
