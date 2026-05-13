/**
 * @file xy_fota_bank.c
 * @brief FOTA Dual Bank Management
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_fota.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief Flash 参数区定义
 */
#define FOTA_PARAM_ADDR     0x08070000  /* 参数区地址 */
#define FOTA_MAGIC          0xF0A512  /* 魔数 */

/**
 * @brief FOTA 参数结构
 */
typedef struct {
    uint32_t magic;                     /* 魔数 */
    uint8_t active_slot;                /* 当前活动 Slot */
    uint8_t slot0_valid;                /* Slot 0 有效性 */
    uint8_t slot1_valid;                /* Slot 1 有效性 */
    uint8_t reserved[5];                /* 保留 */
    uint32_t crc;                       /* CRC32 校验 */
} xy_fota_param_t;

/**
 * @brief 读取参数
 */
static int fota_read_param(xy_fota_param_t *param)
{
    if (!param) {
        return -1;
    }
    
    /* 从 Flash 读取参数 */
    memcpy(param, (void *)FOTA_PARAM_ADDR, sizeof(xy_fota_param_t));
    
    /* 验证魔数 */
    if (param->magic != FOTA_MAGIC) {
        return -1;
    }
    
    /* 验证 CRC */
    uint32_t crc = xy_fota_calc_crc32((uint8_t*)param, sizeof(xy_fota_param_t) - 4);
    if (crc != param->crc) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief 写入参数
 */
static int fota_write_param(const xy_fota_param_t *param)
{
    xy_fota_param_t write_param;
    
    if (!param) {
        return -1;
    }
    
    /* 准备写入数据 */
    memcpy(&write_param, param, sizeof(xy_fota_param_t));
    write_param.magic = FOTA_MAGIC;
    write_param.crc = xy_fota_calc_crc32((uint8_t*)&write_param, sizeof(write_param) - 4);
    
    /* 写入 Flash 参数区 */
    /* xy_fota_flash_write(FOTA_PARAM_ADDR, (uint8_t*)&write_param, sizeof(write_param)); */
    
    return 0;
}

/**
 * @brief 实现双 Bank 交换逻辑
 */
int xy_fota_bank_swap(xy_fota_t *fota)
{
    xy_fota_param_t param;
    uint8_t new_active;
    
    if (!fota) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 读取当前参数 */
    if (fota_read_param(&param) != 0) {
        /* 参数无效，使用默认值 */
        param.active_slot = 0;
        param.slot0_valid = 1;
        param.slot1_valid = 0;
    }
    
    /* 切换活动 Slot */
    new_active = (param.active_slot == 0) ? 1 : 0;
    
    /* 验证新 Slot 有效性 */
    if (new_active == 0 && !param.slot0_valid) {
        xy_log_e("Slot 0 is not valid\n");
        return XY_FOTA_ERROR;
    }
    if (new_active == 1 && !param.slot1_valid) {
        xy_log_e("Slot 1 is not valid\n");
        return XY_FOTA_ERROR;
    }
    
    /* 更新活动 Slot */
    param.active_slot = new_active;
    
    /* 写入参数 */
    fota_write_param(&param);
    
    xy_log_i("FOTA bank swapped to Slot %d\n", new_active);
    
    /* 触发系统复位 */
    /* NVIC_SystemReset(); */
    
    return XY_FOTA_OK;
}

/**
 * @brief 标记 Slot 为有效
 */
int xy_fota_bank_mark_valid(xy_fota_t *fota, uint8_t slot)
{
    xy_fota_param_t param;
    
    if (!fota || slot > 1) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 读取当前参数 */
    if (fota_read_param(&param) != 0) {
        /* 参数无效，使用默认值 */
        param.active_slot = 0;
        param.slot0_valid = 1;
        param.slot1_valid = 0;
    }
    
    /* 标记 Slot 有效 */
    if (slot == 0) {
        param.slot0_valid = 1;
    } else {
        param.slot1_valid = 1;
    }
    
    /* 写入参数 */
    fota_write_param(&param);
    
    xy_log_i("Slot %d marked as valid\n", slot);
    
    return XY_FOTA_OK;
}

/**
 * @brief 检查 Slot 有效性
 */
int xy_fota_bank_is_valid(xy_fota_t *fota, uint8_t slot, bool *valid)
{
    xy_fota_param_t param;
    
    if (!fota || !valid || slot > 1) {
        return XY_FOTA_INVALID_PARAM;
    }
    
    /* 读取参数 */
    if (fota_read_param(&param) != 0) {
        /* 参数无效，默认 Slot 0 有效 */
        *valid = (slot == 0);
        return XY_FOTA_OK;
    }
    
    /* 返回有效性 */
    if (slot == 0) {
        *valid = (param.slot0_valid != 0);
    } else {
        *valid = (param.slot1_valid != 0);
    }
    
    return XY_FOTA_OK;
}
