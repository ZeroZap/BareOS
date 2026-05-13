/**
 * @file xy_fota.c
 * @brief Firmware Over-The-Air Update Implementation
 * @version 1.1.0
 * @date 2026-04-02
 */

#include "xy_fota.h"
#include <string.h>
#include <stdlib.h>

/* FOTA Magic Number */
#define FOTA_MAGIC          0x464F5441
#define FOTA_HEADER_SIZE   sizeof(xy_fota_header_t)

static xy_fota_t g_fota;

/* Forward declarations for static functions */
static int xy_fota_flash_write(xy_fota_t *fota, uint32_t offset, const uint8_t *data, uint32_t size);
static int xy_fota_flash_read(xy_fota_t *fota, uint32_t offset, uint8_t *data, uint32_t size);
static int xy_fota_flash_erase(xy_fota_t *fota);
static int xy_fota_backup_write(xy_fota_t *fota, uint32_t offset, const uint8_t *data, uint32_t size);
static int xy_fota_backup_read(xy_fota_t *fota, uint32_t offset, uint8_t *data, uint32_t size);
static int xy_fota_do_backup(xy_fota_t *fota);
static int xy_fota_do_restore(xy_fota_t *fota);
static int xy_fota_apply_delta(xy_fota_t *fota);

/* ==================== CRC32 ==================== */

uint32_t xy_fota_calc_crc32(const uint8_t *data, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (uint32_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }
    
    return ~crc;
}

/* ==================== Header Validation ==================== */

bool xy_fota_validate_header(const xy_fota_header_t *header)
{
    if (!header) {
        return false;
    }
    
    if (header->magic != FOTA_MAGIC) {
        return false;
    }
    
    if (header->image_size == 0 || header->image_size > XY_FOTA_MAX_IMAGE_SIZE) {
        return false;
    }
    
    return true;
}

/* ==================== Version Validation (防回滚) ==================== */

bool xy_fota_validate_version(xy_fota_t *fota, uint32_t version)
{
    if (!fota || !fota->initialized) {
        return false;
    }
    
    /* 启用回滚保护且设置了最低版本 */
    if (fota->config.enable_rollback && fota->config.min_version > 0) {
        if (version < fota->config.min_version) {
            return false;
        }
    }
    
    /* 新版本必须比当前版本高 */
    if (version <= fota->header.version) {
        /* 允许相同版本重烧 */
        return (version == fota->header.version);
    }
    
    return true;
}

/* ==================== Initialization ==================== */

int xy_fota_init(xy_fota_t *fota, const xy_fota_config_t *config)
{
    if (!fota || !config) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 参数校验 */
    if (config->mode == XY_FOTA_MODE_SINGLE_SLOT) {
        if (config->backup_addr == 0 || config->backup_size == 0) {
            return XY_FOTA_INVALID_PARAM;  /* 单槽模式需要备份区 */
        }
    }
    
    if (config->slot_count == 0 || config->slot_count > 2) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (config->slot_size == 0) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    memset(fota, 0, sizeof(*fota));
    memcpy(&fota->config, config, sizeof(*config));
    fota->state = XY_FOTA_STATE_IDLE;
    fota->initialized = true;
    
    return XY_FOTA_OK;
}

int xy_fota_deinit(xy_fota_t *fota)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->initialized = false;
    fota->state = XY_FOTA_STATE_IDLE;
    
    return XY_FOTA_OK;
}

/* ==================== Download ==================== */

int xy_fota_start_download(xy_fota_t *fota, uint32_t version, uint32_t size, bool is_delta)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (size == 0 || size > XY_FOTA_MAX_IMAGE_SIZE) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 版本校验 (防回滚) */
    if (!xy_fota_validate_version(fota, version)) {
        return XY_FOTA_VERSION_ERROR;
    }
    
    /* 增量升级检查 */
    if (is_delta && fota->config.mode != XY_FOTA_MODE_DELTA) {
        /* 自动切换到增量模式 */
        fota->config.mode = XY_FOTA_MODE_DELTA;
    }
    
    fota->header.magic = FOTA_MAGIC;
    fota->header.version = version;
    fota->header.image_size = size;
    fota->header.crc32 = 0;
    fota->header.delta_size = is_delta ? size : 0;
    fota->header.flags = is_delta ? XY_FOTA_FLAG_DELTA : 0;
    fota->downloaded_bytes = 0;
    fota->state = XY_FOTA_STATE_DOWNLOADING;
    
    return XY_FOTA_OK;
}

int xy_fota_download_chunk(xy_fota_t *fota, const uint8_t *data, uint32_t size)
{
    int ret;
    
    if (!fota || !data || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (fota->state != XY_FOTA_STATE_DOWNLOADING) {
        return XY_FOTA_IN_PROGRESS;
    }
    
    /* 检查是否超量 */
    if (fota->downloaded_bytes + size > fota->header.image_size) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 根据模式写入不同位置 */
    if (fota->config.mode == XY_FOTA_MODE_DELTA) {
        /* 增量模式：写入临时区域 */
        ret = xy_fota_flash_write(fota, fota->downloaded_bytes, data, size);
    } else {
        /* 全量模式：写入目标槽位 */
        ret = xy_fota_flash_write(fota, fota->downloaded_bytes, data, size);
    }
    
    if (ret != XY_FOTA_OK) {
        fota->state = XY_FOTA_STATE_ERROR;
        return XY_FOTA_FLASH_ERROR;
    }
    
    fota->downloaded_bytes += size;
    
    /* 进度回调 */
    if (fota->progress_cb) {
        fota->progress_cb(fota->downloaded_bytes, fota->header.image_size, fota->user_data);
    }
    
    /* 下载完成 */
    if (fota->downloaded_bytes >= fota->header.image_size) {
        if (fota->config.mode == XY_FOTA_MODE_DELTA) {
            fota->state = XY_FOTA_STATE_PATCHING;
        } else {
            fota->state = XY_FOTA_STATE_VALIDATING;
        }
    }
    
    return XY_FOTA_OK;
}

int xy_fota_finish_download(xy_fota_t *fota)
{
    int ret;
    
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (fota->state == XY_FOTA_STATE_PATCHING) {
        /* 增量模式：应用补丁 */
        if (xy_fota_apply_delta(fota) != XY_FOTA_OK) {
            fota->state = XY_FOTA_STATE_ERROR;
            return XY_FOTA_DELTA_ERROR;
        }
    }
    
    if (fota->state != XY_FOTA_STATE_VALIDATING && 
        fota->state != XY_FOTA_STATE_PATCHING) {
        return XY_FOTA_ERROR;
    }
    
    /* 在单槽模式下，下载完成后需要备份当前固件 */
    if (fota->config.mode == XY_FOTA_MODE_SINGLE_SLOT) {
        fota->state = XY_FOTA_STATE_BACKUP;
        ret = xy_fota_do_backup(fota);
        if (ret != XY_FOTA_OK) {
            fota->state = XY_FOTA_STATE_ERROR;
            return ret;
        }
    }
    
    fota->state = XY_FOTA_STATE_COMPLETE;
    
    return XY_FOTA_OK;
}

/* ==================== Update ==================== */

int xy_fota_start_update(xy_fota_t *fota)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (fota->state != XY_FOTA_STATE_COMPLETE) {
        return XY_FOTA_ERROR;
    }
    
    fota->state = XY_FOTA_STATE_UPDATING;
    
    /* 根据模式执行更新 */
    switch (fota->config.mode) {
        case XY_FOTA_MODE_DUAL_BANK:
            /* 双槽模式：切换启动槽位 */
            fota->current_slot = 1 - fota->current_slot;
            break;
            
        case XY_FOTA_MODE_SINGLE_SLOT:
            /* 单槽模式：更新固件在原位置 */
            break;
            
        case XY_FOTA_MODE_DELTA:
            /* 增量模式：补丁已应用 */
            break;
            
        default:
            break;
    }
    
    fota->state = XY_FOTA_STATE_VERIFYING;
    
    /* 验证新固件 (实际实现应跳转到新固件运行) */
    /* 这里仅模拟完成 */
    fota->state = XY_FOTA_STATE_COMPLETE;
    
    return XY_FOTA_OK;
}

/* ==================== Rollback ==================== */

int xy_fota_rollback(xy_fota_t *fota)
{
    int ret;
    
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    if (!fota->config.enable_rollback) {
        return XY_FOTA_ERROR;  /* 未启用回滚功能 */
    }
    
    fota->state = XY_FOTA_STATE_ROLLBACK;
    
    /* 从备份区恢复 */
    ret = xy_fota_do_restore(fota);
    if (ret != XY_FOTA_OK) {
        fota->state = XY_FOTA_STATE_ERROR;
        return ret;
    }
    
    fota->state = XY_FOTA_STATE_COMPLETE;
    
    return XY_FOTA_OK;
}

bool xy_fota_needs_rollback(xy_fota_t *fota)
{
    if (!fota || !fota->initialized) {
        return false;
    }
    
    /* 检查是否需要回滚条件：
     * 1. 启用了回滚功能
     * 2. 上电检测到连续多次启动失败
     * 3. 固件标记为损坏
     * 
     * 此处简化实现：检查标志位
     */
    
    /* 实际实现应检查 Flash 中的回滚标志 */
    return false;
}

uint32_t xy_fota_get_current_version(xy_fota_t *fota)
{
    if (!fota) {
        return 0;
    }
    
    return fota->header.version;
}

/* ==================== Flash Operations ==================== */

static int xy_fota_flash_write(xy_fota_t *fota, uint32_t offset, const uint8_t *data, uint32_t size)
{
    if (!fota || !fota->flash_ops || !fota->flash_ops->write) {
        return XY_FOTA_FLASH_ERROR;
    }
    
    uint32_t flash_addr;
    
    if (fota->config.mode == XY_FOTA_MODE_SINGLE_SLOT && 
        fota->state == XY_FOTA_STATE_DOWNLOADING) {
        /* 单槽下载模式：写入备份区 */
        flash_addr = fota->config.backup_addr + offset;
    } else {
        /* 写入目标槽位 */
        flash_addr = fota->config.flash_base_addr + 
                     (fota->current_slot * fota->config.slot_size) + 
                     offset;
    }
    
    return fota->flash_ops->write(flash_addr, data, size);
}

static int xy_fota_flash_read(xy_fota_t *fota, uint32_t offset, uint8_t *data, uint32_t size)
{
    if (!fota || !fota->flash_ops || !fota->flash_ops->read) {
        return XY_FOTA_FLASH_ERROR;
    }
    
    uint32_t flash_addr = fota->config.flash_base_addr + 
                         (fota->current_slot * fota->config.slot_size) + 
                         offset;
    
    return fota->flash_ops->read(flash_addr, data, size);
}

static int xy_fota_flash_erase(xy_fota_t *fota)
{
    if (!fota || !fota->flash_ops || !fota->flash_ops->erase) {
        return XY_FOTA_FLASH_ERROR;
    }
    
    uint32_t flash_addr = fota->config.flash_base_addr + 
                         (fota->current_slot * fota->config.slot_size);
    
    return fota->flash_ops->erase(flash_addr, fota->config.slot_size);
}

/* ==================== Backup Operations (单槽模式) ==================== */

static int xy_fota_backup_write(xy_fota_t *fota, uint32_t offset, const uint8_t *data, uint32_t size)
{
    /* 优先使用专用备份 Flash */
    if (fota->backup_flash_ops && fota->backup_flash_ops->write) {
        uint32_t addr = fota->config.backup_addr + offset;
        return fota->backup_flash_ops->write(addr, data, size);
    }
    
    /* 否则使用主 Flash */
    if (!fota->flash_ops || !fota->flash_ops->write) {
        return XY_FOTA_FLASH_ERROR;
    }
    
    uint32_t addr = fota->config.backup_addr + offset;
    return fota->flash_ops->write(addr, data, size);
}

static int xy_fota_backup_read(xy_fota_t *fota, uint32_t offset, uint8_t *data, uint32_t size)
{
    if (fota->backup_flash_ops && fota->backup_flash_ops->read) {
        uint32_t addr = fota->config.backup_addr + offset;
        return fota->backup_flash_ops->read(addr, data, size);
    }
    
    if (!fota->flash_ops || !fota->flash_ops->read) {
        return XY_FOTA_FLASH_ERROR;
    }
    
    uint32_t addr = fota->config.backup_addr + offset;
    return fota->flash_ops->read(addr, data, size);
}

static int xy_fota_do_backup(xy_fota_t *fota)
{
    int ret;
    uint8_t *buffer;
    uint32_t size;
    
    if (!fota || fota->config.backup_size == 0) {
        return XY_FOTA_NO_BACKUP;
    }
    
    /* 分配缓冲区 */
    buffer = malloc(256);
    if (!buffer) {
        return XY_FOTA_NO_MEM;
    }
    
    /* 读取当前固件头部，保存版本信息 */
    ret = xy_fota_backup_read(fota, 0, buffer, FOTA_HEADER_SIZE);
    if (ret != XY_FOTA_OK) {
        free(buffer);
        return ret;
    }
    
    /* 保存到备份区 (偏移存储) */
    memcpy(buffer, &fota->header, sizeof(fota->header));
    ret = xy_fota_backup_write(fota, 0, buffer, FOTA_HEADER_SIZE);
    if (ret != XY_FOTA_OK) {
        free(buffer);
        return ret;
    }
    
    /* 备份固件主体 */
    size = fota->header.image_size;
    for (uint32_t i = 0; i < size; i += 256) {
        uint32_t chunk = (size - i) > 256 ? 256 : (size - i);
        
        ret = xy_fota_flash_read(fota, i, buffer, chunk);
        if (ret != XY_FOTA_OK) {
            free(buffer);
            return ret;
        }
        
        ret = xy_fota_backup_write(fota, FOTA_HEADER_SIZE + i, buffer, chunk);
        if (ret != XY_FOTA_OK) {
            free(buffer);
            return ret;
        }
    }
    
    /* 保存备份版本号 */
    fota->backup_version = fota->header.version;
    
    free(buffer);
    return XY_FOTA_OK;
}

static int xy_fota_do_restore(xy_fota_t *fota)
{
    int ret;
    uint8_t *buffer;
    xy_fota_header_t header;
    uint32_t size;
    
    if (!fota || fota->config.backup_size == 0) {
        return XY_FOTA_NO_BACKUP;
    }
    
    /* 读取备份头 */
    buffer = malloc(FOTA_HEADER_SIZE);
    if (!buffer) {
        return XY_FOTA_NO_MEM;
    }
    
    ret = xy_fota_backup_read(fota, 0, buffer, FOTA_HEADER_SIZE);
    if (ret != XY_FOTA_OK) {
        free(buffer);
        return ret;
    }
    
    memcpy(&header, buffer, sizeof(header));
    free(buffer);
    
    /* 恢复固件 */
    buffer = malloc(256);
    if (!buffer) {
        return XY_FOTA_NO_MEM;
    }
    
    size = header.image_size;
    for (uint32_t i = 0; i < size; i += 256) {
        uint32_t chunk = (size - i) > 256 ? (size - i) : 256;
        
        ret = xy_fota_backup_read(fota, FOTA_HEADER_SIZE + i, buffer, chunk);
        if (ret != XY_FOTA_OK) {
            free(buffer);
            return ret;
        }
        
        ret = xy_fota_flash_write(fota, i, buffer, chunk);
        if (ret != XY_FOTA_OK) {
            free(buffer);
            return ret;
        }
    }
    
    /* 更新版本信息 */
    fota->header.version = header.version;
    
    free(buffer);
    return XY_FOTA_OK;
}

/* ==================== Delta Patching ==================== */

static int xy_fota_apply_delta(xy_fota_t *fota)
{
    /* 增量补丁应用逻辑
     * 实际实现需要:
     * 1. 读取增量补丁数据
     * 2. 对当前固件应用 bsdiff/bspatch
     * 3. 生成新固件
     * 
     * 此处为框架实现
     */
    
    /* 检查是否有补丁回调 */
    /* 如果没有，回退到全量模式 */
    if (!fota->progress_cb) {
        return XY_FOTA_DELTA_ERROR;
    }
    
    fota->state = XY_FOTA_STATE_VALIDATING;
    return XY_FOTA_OK;
}

/* ==================== State & Progress ==================== */

xy_fota_state_t xy_fota_get_state(xy_fota_t *fota)
{
    if (!fota) {
        return XY_FOTA_STATE_ERROR;
    }
    
    return fota->state;
}

uint8_t xy_fota_get_progress(xy_fota_t *fota)
{
    if (!fota || fota->header.image_size == 0) {
        return 0;
    }
    
    return (fota->downloaded_bytes * 100) / fota->header.image_size;
}

int xy_fota_set_progress_callback(xy_fota_t *fota, xy_fota_progress_cb cb, void *user_data)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->progress_cb = cb;
    fota->user_data = user_data;
    
    return XY_FOTA_OK;
}

int xy_fota_set_patch_callback(xy_fota_t *fota, xy_fota_patch_cb cb, void *user_data)
{
    /* 占位：增量补丁回调设置 */
    (void)fota;
    (void)cb;
    (void)user_data;
    return XY_FOTA_OK;
}

/* ==================== Control ==================== */

int xy_fota_cancel(xy_fota_t *fota)
{
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->state = XY_FOTA_STATE_IDLE;
    fota->downloaded_bytes = 0;
    
    return XY_FOTA_OK;
}

int xy_fota_reset(xy_fota_t *fota)
{
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->state = XY_FOTA_STATE_IDLE;
    fota->downloaded_bytes = 0;
    memset(&fota->header, 0, sizeof(fota->header));
    
    return XY_FOTA_OK;
}

/* ==================== Flash Ops Setters ==================== */

int xy_fota_set_flash_ops(xy_fota_t *fota, const xy_fota_flash_ops_t *ops)
{
    if (!fota || !ops) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->flash_ops = ops;
    return XY_FOTA_OK;
}

int xy_fota_set_backup_flash_ops(xy_fota_t *fota, const xy_fota_flash_ops_t *ops)
{
    if (!fota || !ops) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    fota->backup_flash_ops = ops;
    return XY_FOTA_OK;
}

/* ==================== Validation ==================== */

int xy_fota_validate(xy_fota_t *fota)
{
    int ret;
    uint32_t crc;
    uint8_t *buffer;
    
    if (!fota || !fota->initialized) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    buffer = malloc(256);
    if (!buffer) {
        return XY_FOTA_NO_MEM;
    }
    
    /* 计算 CRC */
    crc = 0;
    for (uint32_t i = 0; i < fota->header.image_size; i += 256) {
        uint32_t size = (fota->header.image_size - i) > 256 ? 256 : (fota->header.image_size - i);
        
        ret = xy_fota_flash_read(fota, i, buffer, size);
        if (ret != XY_FOTA_OK) {
            free(buffer);
            return XY_FOTA_FLASH_ERROR;
        }
        
        crc = xy_fota_calc_crc32(buffer, size);
    }
    
    free(buffer);
    
    /* 验证 CRC */
    if (crc != fota->header.crc32) {
        return XY_FOTA_CRC_ERROR;
    }
    
    return XY_FOTA_OK;
}
