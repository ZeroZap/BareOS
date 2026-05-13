/**
 * SPDX-License-Identifier: MIT
 * @file    xy_pmbus.c
 * @brief   PMBus Protocol Implementation
 * @version 1.0.0
 */

#include "xy_pmbus.h"
#include "xy_log.h"
#include <string.h>
#include <stdlib.h>

/* ==================== 静态变量 ==================== */
#define MAX_PMBUS_DEVICES  8
static pmbus_device_t *g_pmbus_devices[MAX_PMBUS_DEVICES];
static uint8_t g_pmbus_count = 0;

/* ==================== 工具函数 ==================== */
const char *pmbus_err_str(smbus_err_t err)
{
    return smbus_err_str(err);
}

void pmbus_dump_status(const pmbus_status_t *status)
{
    if (status == NULL) return;

    printf("PMBus Status:\r\n");

    /* VOUT status */
    printf("  VOUT: ");
    if (status->vout_ov_fault) printf("OV_FAULT ");
    if (status->vout_ov_warning) printf("OV_WARN ");
    if (status->vout_uv_warning) printf("UV_WARN ");
    if (status->vout_uv_fault) printf("UV_FAULT ");
    if (status->vout_margin_high) printf("MARGIN_HIGH ");
    if (status->vout_margin_low) printf("MARGIN_LOW ");
    printf("\r\n");

    /* IOUT status */
    printf("  IOUT: ");
    if (status->iout_oc_fault) printf("OC_FAULT ");
    if (status->iout_oc_warning) printf("OC_WARN ");
    if (status->iout_uc_fault) printf("UC_FAULT ");
    printf("\r\n");

    /* INPUT status */
    printf("  INPUT: ");
    if (status->vin_ov_fault) printf("OV_FAULT ");
    if (status->vin_ov_warning) printf("OV_WARN ");
    if (status->vin_uv_warning) printf("UV_WARN ");
    if (status->vin_uv_fault) printf("UV_FAULT ");
    printf("\r\n");

    /* Temperature status */
    printf("  TEMP: ");
    if (status->temp_ot_fault) printf("OT_FAULT ");
    if (status->temp_ot_warning) printf("OT_WARN ");
    if (status->temp_ut_warning) printf("UT_WARN ");
    printf("\r\n");

    /* General status */
    printf("  GENERAL: ");
    if (status->cml_fault) printf("CML ");
    if (status->memory_fault) printf("MEM ");
    if (status->processor_fault) printf("CPU ");
    if (status->off) printf("OFF ");
    if (status->busy) printf("BUSY ");
    printf("\r\n");
}

/* ==================== 设备管理 ==================== */
smbus_err_t pmbus_register(pmbus_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL) {
        return SMBUS_EINVAL;
    }

    if (g_pmbus_count >= MAX_PMBUS_DEVICES) {
        return SMBUS_ERROR;
    }

    g_pmbus_devices[g_pmbus_count++] = dev;
    return SMBUS_EOK;
}

smbus_err_t pmbus_unregister(pmbus_device_t *dev)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    for (int i = 0; i < g_pmbus_count; i++) {
        if (g_pmbus_devices[i] == dev) {
            for (int j = i; j < g_pmbus_count - 1; j++) {
                g_pmbus_devices[j] = g_pmbus_devices[j + 1];
            }
            g_pmbus_count--;
            return SMBUS_EOK;
        }
    }

    return SMBUS_ENODEV;
}

pmbus_device_t *pmbus_find(const char *name)
{
    if (name == NULL) return NULL;

    for (int i = 0; i < g_pmbus_count; i++) {
        if (strcmp(g_pmbus_devices[i]->name, name) == 0) {
            return g_pmbus_devices[i];
        }
    }
    return NULL;
}

/* ==================== 基础操作 ==================== */
smbus_err_t pmbus_init(pmbus_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->init == NULL) {
        return SMBUS_EINVAL;
    }

    /* 初始化 SMBus */
    if (dev->smbus) {
        smbus_init(dev->smbus);
    }

    return dev->ops->init(dev);
}

smbus_err_t pmbus_deinit(pmbus_device_t *dev)
{
    if (dev == NULL || dev->ops == NULL || dev->ops->deinit == NULL) {
        return SMBUS_EINVAL;
    }

    return dev->ops->deinit(dev);
}

/* ==================== 状态解析 ==================== */
smbus_err_t pmbus_parse_status_word(uint16_t word, pmbus_status_t *status)
{
    if (status == NULL) {
        return SMBUS_EINVAL;
    }

    memset(status, 0, sizeof(pmbus_status_t));

    /* VOUT status (byte 1) */
    status->vout_ov_fault = (word & (1 << 0)) != 0;
    status->vout_ov_warning = (word & (1 << 1)) != 0;
    status->vout_margin_high = (word & (1 << 2)) != 0;
    status->vout_margin_low = (word & (1 << 3)) != 0;
    status->vout_uv_warning = (word & (1 << 5)) != 0;
    status->vout_uv_fault = (word & (1 << 6)) != 0;

    /* IOUT status */
    status->iout_oc_warning = (word & (1 << 7)) != 0;
    status->iout_oc_fault = (word & (1 << 8)) != 0;
    status->iout_uc_fault = (word & (1 << 9)) != 0;

    /* INPUT status */
    status->vin_ov_fault = (word & (1 << 11)) != 0;
    status->vin_ov_warning = (word & (1 << 12)) != 0;
    status->vin_uv_warning = (word & (1 << 13)) != 0;
    status->vin_uv_fault = (word & (1 << 14)) != 0;

    /* Temperature status */
    status->temp_ot_fault = (word & (1 << 15)) != 0;

    /* Additional status (high byte) */
    /* 需要从 STATUS_MFR_SPECIFIC 等读取 */

    return SMBUS_EOK;
}

/* ==================== 读取命令 ==================== */
smbus_err_t pmbus_read_vout(pmbus_device_t *dev, float *voltage)
{
    if (dev == NULL || voltage == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_vout) {
        return dev->ops->read_vout(dev, voltage);
    }

    /* 默认实现: 通过 SMBus 读取 VOUT_READ (0x2B) */
    uint8_t addr = dev->config.smbus.addr;

    /* 读取 VOUT_MODE */
    uint8_t vout_mode;
    smbus_read_byte(dev->smbus, addr, PMBUS_CMD_VOUT_MODE, &vout_mode);

    /* 读取 VOUT_READ */
    uint16_t raw;
    smbus_err_t err = smbus_read_word(dev->smbus, addr, PMBUS_CMD_VOUT_READ, &raw);
    if (err != SMBUS_EOK) {
        return err;
    }

    /* 根据 VOUT_MODE 转换 */
    pmbus_vout_mode_t mode = (pmbus_vout_mode_t)(vout_mode & 0x1F);
    switch (mode) {
        case PMBUS_VOUT_MODE_LINEAR:
        default:
            *voltage = pmbus_linear_to_float(raw);
            break;
        case PMBUS_VOUT_MODE_VID:
            *voltage = pmbus_vid_to_voltage((uint8_t)raw);
            break;
    }

    return SMBUS_EOK;
}

smbus_err_t pmbus_read_iout(pmbus_device_t *dev, float *current)
{
    if (dev == NULL || current == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_iout) {
        return dev->ops->read_iout(dev, current);
    }

    uint8_t addr = dev->config.smbus.addr;
    uint16_t raw;

    smbus_err_t err = smbus_read_word(dev->smbus, addr, PMBUS_CMD_IOUT_READ, &raw);
    if (err != SMBUS_EOK) {
        return err;
    }

    *current = pmbus_linear_to_float(raw);
    return SMBUS_EOK;
}

smbus_err_t pmbus_read_vin(pmbus_device_t *dev, float *voltage)
{
    if (dev == NULL || voltage == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_vin) {
        return dev->ops->read_vin(dev, voltage);
    }

    uint8_t addr = dev->config.smbus.addr;
    uint16_t raw;

    smbus_err_t err = smbus_read_word(dev->smbus, addr, PMBUS_CMD_READ_VIN, &raw);
    if (err != SMBUS_EOK) {
        return err;
    }

    *voltage = pmbus_linear_to_float(raw);
    return SMBUS_EOK;
}

smbus_err_t pmbus_read_pin(pmbus_device_t *dev, float *power)
{
    if (dev == NULL || power == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_pin) {
        return dev->ops->read_pin(dev, power);
    }

    uint8_t addr = dev->config.smbus.addr;
    uint16_t raw;

    smbus_err_t err = smbus_read_word(dev->smbus, addr, PMBUS_CMD_READ_PIN, &raw);
    if (err != SMBUS_EOK) {
        return err;
    }

    *power = pmbus_linear_to_float(raw);
    return SMBUS_EOK;
}

smbus_err_t pmbus_read_pout(pmbus_device_t *dev, float *power)
{
    if (dev == NULL || power == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_pout) {
        return dev->ops->read_pout(dev, power);
    }

    uint8_t addr = dev->config.smbus.addr;
    uint16_t raw;

    smbus_err_t err = smbus_read_word(dev->smbus, addr, PMBUS_CMD_READ_POUT, &raw);
    if (err != SMBUS_EOK) {
        return err;
    }

    *power = pmbus_linear_to_float(raw);
    return SMBUS_EOK;
}

smbus_err_t pmbus_read_temp(pmbus_device_t *dev, uint8_t sensor, float *temp)
{
    if (dev == NULL || temp == NULL || sensor > 2) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_temp) {
        return dev->ops->read_temp(dev, sensor, temp);
    }

    uint8_t addr = dev->config.smbus.addr;
    uint8_t cmd = (sensor == 0) ? PMBUS_CMD_TEMP_1_INPUT : PMBUS_CMD_TEMP_2_INPUT;
    uint16_t raw;

    smbus_err_t err = smbus_read_word(dev->smbus, addr, cmd, &raw);
    if (err != SMBUS_EOK) {
        return err;
    }

    *temp = pmbus_linear_to_float(raw);
    return SMBUS_EOK;
}

/* ==================== 写入命令 ==================== */
smbus_err_t pmbus_write_vout_command(pmbus_device_t *dev, float voltage)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->write_vout_command) {
        return dev->ops->write_vout_command(dev, voltage);
    }

    uint8_t addr = dev->config.smbus.addr;
    uint16_t raw = pmbus_float_to_linear(voltage);

    return smbus_write_word(dev->smbus, addr, PMBUS_CMD_VOUT_COMMAND, raw);
}

smbus_err_t pmbus_set_operation(pmbus_device_t *dev, pmbus_operation_t op)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->set_operation) {
        return dev->ops->set_operation(dev, op);
    }

    uint8_t addr = dev->config.smbus.addr;
    return smbus_send_byte(dev->smbus, addr, (uint8_t)op);
}

smbus_err_t pmbus_clear_faults(pmbus_device_t *dev)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->clear_faults) {
        return dev->ops->clear_faults(dev);
    }

    uint8_t addr = dev->config.smbus.addr;
    return smbus_send_byte(dev->smbus, addr, PMBUS_CMD_CLEAR_FAULTS);
}

/* ==================== 状态命令 ==================== */
smbus_err_t pmbus_read_status_word(pmbus_device_t *dev, uint16_t *status)
{
    if (dev == NULL || status == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->read_status_word) {
        return dev->ops->read_status_word(dev, status);
    }

    uint8_t addr = dev->config.smbus.addr;
    return smbus_read_word(dev->smbus, addr, PMBUS_CMD_STATUS_WORD, status);
}

smbus_err_t pmbus_get_status(pmbus_device_t *dev, pmbus_status_t *status)
{
    if (dev == NULL || status == NULL) {
        return SMBUS_EINVAL;
    }

    if (dev->ops && dev->ops->get_status) {
        return dev->ops->get_status(dev, status);
    }

    uint16_t word;
    smbus_err_t err = pmbus_read_status_word(dev, &word);
    if (err != SMBUS_EOK) {
        return err;
    }

    dev->status_word = word;
    return pmbus_parse_status_word(word, status);
}

/* ==================== 配置命令 ==================== */
smbus_err_t pmbus_store_default(pmbus_device_t *dev)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    uint8_t addr = dev->config.smbus.addr;
    return smbus_send_byte(dev->smbus, addr, PMBUS_CMD_STORE_DEFAULT_ALL);
}

smbus_err_t pmbus_restore_default(pmbus_device_t *dev)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    uint8_t addr = dev->config.smbus.addr;
    return smbus_send_byte(dev->smbus, addr, PMBUS_CMD_RESTORE_DEFAULT_ALL);
}

smbus_err_t pmbus_set_page(pmbus_device_t *dev, uint8_t page)
{
    if (dev == NULL) {
        return SMBUS_EINVAL;
    }

    uint8_t addr = dev->config.smbus.addr;
    return smbus_write_byte(dev->smbus, addr, PMBUS_CMD_PAGE, page);
}

/* ==================== 设备 ID ==================== */
smbus_err_t pmbus_read_id(pmbus_device_t *dev, char *model, uint8_t max_len)
{
    if (dev == NULL || model == NULL) {
        return SMBUS_EINVAL;
    }

    uint8_t addr = dev->config.smbus.addr;
    uint8_t buf[32];
    uint8_t len = max_len > 32 ? 32 : max_len;

    smbus_err_t err = smbus_read_block(dev->smbus, addr, PMBUS_CMD_ID_WORD, buf, &len);
    if (err != SMBUS_EOK) {
        return err;
    }

    memcpy(model, buf, len);
    model[len] = '\0';
    return SMBUS_EOK;
}

smbus_err_t pmbus_read_mfr_id(pmbus_device_t *dev, char *mfr_id, uint8_t max_len)
{
    if (dev == NULL || mfr_id == NULL) {
        return SMBUS_EINVAL;
    }

    uint8_t addr = dev->config.smbus.addr;
    uint8_t buf[32];
    uint8_t len = max_len > 32 ? 32 : max_len;

    smbus_err_t err = smbus_read_block(dev->smbus, addr, PMBUS_CMD_MFR_ID, buf, &len);
    if (err != SMBUS_EOK) {
        return err;
    }

    memcpy(mfr_id, buf, len);
    mfr_id[len] = '\0';
    return SMBUS_EOK;
}

/* ==================== 打印所有读数 ==================== */
void pmbus_dump_all_readings(pmbus_device_t *dev)
{
    if (dev == NULL) return;

    printf("=== PMBus Readings: %s ===\r\n", dev->name);

    float value;

    if (pmbus_read_vout(dev, &value) == SMBUS_EOK) {
        printf("  VOUT: %.3f V\r\n", value);
    }

    if (pmbus_read_iout(dev, &value) == SMBUS_EOK) {
        printf("  IOUT: %.3f A\r\n", value);
    }

    if (pmbus_read_vin(dev, &value) == SMBUS_EOK) {
        printf("  VIN:  %.3f V\r\n", value);
    }

    if (pmbus_read_pin(dev, &value) == SMBUS_EOK) {
        printf("  PIN:  %.3f W\r\n", value);
    }

    if (pmbus_read_pout(dev, &value) == SMBUS_EOK) {
        printf("  POUT: %.3f W\r\n", value);
    }

    if (pmbus_read_temp(dev, 0, &value) == SMBUS_EOK) {
        printf("  TEMP1: %.1f C\r\n", value);
    }

    if (pmbus_read_temp(dev, 1, &value) == SMBUS_EOK) {
        printf("  TEMP2: %.1f C\r\n", value);
    }

    pmbus_status_t status;
    if (pmbus_get_status(dev, &status) == SMBUS_EOK) {
        pmbus_dump_status(&status);
    }
}

/* ==================== 默认操作实现 ==================== */
static smbus_err_t default_init(pmbus_device_t *dev)
{
    (void)dev;
    return SMBUS_EOK;
}

static smbus_err_t default_deinit(pmbus_device_t *dev)
{
    (void)dev;
    return SMBUS_EOK;
}

static smbus_err_t default_read_vout(pmbus_device_t *dev, float *voltage)
{
    return pmbus_read_vout(dev, voltage);
}

static smbus_err_t default_read_iout(pmbus_device_t *dev, float *current)
{
    return pmbus_read_iout(dev, current);
}

static smbus_err_t default_read_temp(pmbus_device_t *dev, uint8_t sensor, float *temp)
{
    return pmbus_read_temp(dev, sensor, temp);
}

static smbus_err_t default_write_vout_command(pmbus_device_t *dev, float voltage)
{
    return pmbus_write_vout_command(dev, voltage);
}

static smbus_err_t default_set_operation(pmbus_device_t *dev, pmbus_operation_t op)
{
    return pmbus_set_operation(dev, op);
}

static smbus_err_t default_read_status_word(pmbus_device_t *dev, uint16_t *status)
{
    return pmbus_read_status_word(dev, status);
}

static smbus_err_t default_clear_faults(pmbus_device_t *dev)
{
    return pmbus_clear_faults(dev);
}

static smbus_err_t default_get_status(pmbus_device_t *dev, pmbus_status_t *status)
{
    return pmbus_get_status(dev, status);
}

/* ==================== 默认操作表 ==================== */
const pmbus_ops_t pmbus_default_ops = {
    .init = default_init,
    .deinit = default_deinit,
    .read_vout = default_read_vout,
    .read_iout = default_read_iout,
    .read_vin = NULL,
    .read_pin = NULL,
    .read_pout = NULL,
    .read_temp = default_read_temp,
    .write_vout_command = default_write_vout_command,
    .set_operation = default_set_operation,
    .read_status_word = default_read_status_word,
    .clear_faults = default_clear_faults,
    .get_status = default_get_status,
};