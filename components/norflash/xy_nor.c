#include "xy_nor.h"
#include <string.h>

/* 内部辅助函数 */
static xy_nor_status_t xy_nor_send_command(xy_nor_handle_t *handle, uint8_t cmd,
                                           uint32_t addr, uint8_t addr_len,
                                           uint8_t *data, uint32_t data_len,
                                           bool is_write);

/**
 * @brief 获取默认配置参数
 */
void xy_nor_get_default_config(xy_nor_config_t *config)
{
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(xy_nor_config_t));
    config->clock_freq       = XY_NOR_DEFAULT_CLOCK_FREQ;
    config->dummy_cycles     = XY_NOR_DEFAULT_DUMMY_CYCLES;
    config->drive_strength   = XY_NOR_DEFAULT_DRIVE_STRENGTH;
    config->slew_rate        = XY_NOR_DEFAULT_SLEW_RATE;
    config->quad_enable      = false;
    config->write_protection = false;
    config->timeout_ms       = XY_NOR_DEFAULT_TIMEOUT_MS;
    config->voltage_range    = 1; // 默认3.3V
}

/**
 * @brief 初始化NOR Flash驱动
 */
xy_nor_status_t xy_nor_init(xy_nor_handle_t *handle,
                            const xy_nor_config_t *config)
{
    xy_nor_status_t status;

    if (handle == NULL || config == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    // 复制配置参数
    memcpy(&handle->config, config, sizeof(xy_nor_config_t));
    handle->is_initialized = false;

    // 初始化硬件
    handle->hw_handle = xy_nor_hw_init(config);
    if (handle->hw_handle == NULL) {
        return XY_NOR_ERROR;
    }

    // 延时等待芯片稳定
    xy_nor_hw_delay_ms(10);

    // 配置电气参数
    status = xy_nor_configure_electrical_params(handle, config);
    if (status != XY_NOR_OK) {
        xy_nor_hw_deinit(handle->hw_handle);
        return status;
    }

    // 退出掉电模式（如果芯片在掉电模式）
    status = xy_nor_release_power_down(handle);
    if (status != XY_NOR_OK) {
        xy_nor_hw_deinit(handle->hw_handle);
        return status;
    }

    // 读取设备信息
    status = xy_nor_read_info(handle);
    if (status != XY_NOR_OK) {
        xy_nor_hw_deinit(handle->hw_handle);
        return status;
    }

    // 等待芯片就绪
    status = xy_nor_wait_ready(handle);
    if (status != XY_NOR_OK) {
        xy_nor_hw_deinit(handle->hw_handle);
        return status;
    }

    handle->is_initialized = true;
    return XY_NOR_OK;
}

/**
 * @brief 去初始化NOR Flash驱动
 */
xy_nor_status_t xy_nor_deinit(xy_nor_handle_t *handle)
{
    if (handle == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    // 进入掉电模式以降低功耗
    xy_nor_power_down(handle);

    // 去初始化硬件
    if (handle->hw_handle != NULL) {
        xy_nor_hw_deinit(handle->hw_handle);
        handle->hw_handle = NULL;
    }

    handle->is_initialized = false;
    return XY_NOR_OK;
}

/**
 * @brief 配置电气参数
 */
xy_nor_status_t
xy_nor_configure_electrical_params(xy_nor_handle_t *handle,
                                   const xy_nor_config_t *config)
{
    xy_nor_status_t status;
    uint8_t status_reg = 0;

    if (handle == NULL || config == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    // 读取当前状态寄存器
    status = xy_nor_read_status(handle, &status_reg);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 配置写保护
    if (config->write_protection) {
        status_reg |=
            (XY_NOR_STATUS_BP0 | XY_NOR_STATUS_BP1 | XY_NOR_STATUS_BP2);
    } else {
        status_reg &=
            ~(XY_NOR_STATUS_BP0 | XY_NOR_STATUS_BP1 | XY_NOR_STATUS_BP2);
    }

    // 配置四线模式（某些芯片需要特殊配置）
    if (config->quad_enable) {
        // 注意：不同厂商的四线使能方式不同
        // 这里提供通用接口，具体实现需根据芯片手册调整
        status_reg |= (1 << 6); // 示例：某些芯片使用SR的bit6作为QE位
    }

    // 写入状态寄存器
    status = xy_nor_write_status(handle, status_reg);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 等待配置完成
    status = xy_nor_wait_ready(handle);

    return status;
}

/**
 * @brief 读取设备信息
 */
xy_nor_status_t xy_nor_read_info(xy_nor_handle_t *handle)
{
    xy_nor_status_t status;
    uint8_t id_data[3];

    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    // 读取JEDEC ID
    status = xy_nor_send_command(
        handle, XY_NOR_CMD_READ_ID, 0, 0, id_data, 3, false);
    if (status != XY_NOR_OK) {
        return status;
    }

    handle->info.jedec_id =
        ((uint32_t)id_data[0] << 16) | ((uint32_t)id_data[1] << 8) | id_data[2];

    // 根据JEDEC ID识别制造商和容量
    uint8_t manufacturer_id = id_data[0];
    uint8_t memory_type     = id_data[1];
    uint8_t capacity_code   = id_data[2];

    // 识别制造商
    switch (manufacturer_id) {
    case 0xEF:
        strcpy(handle->info.manufacturer, "Winbond");
        break;
    case 0xC2:
        strcpy(handle->info.manufacturer, "Macronix");
        break;
    case 0x20:
        strcpy(handle->info.manufacturer, "Micron");
        break;
    case 0x01:
        strcpy(handle->info.manufacturer, "Spansion");
        break;
    case 0xBF:
        strcpy(handle->info.manufacturer, "SST");
        break;
    default:
        strcpy(handle->info.manufacturer, "Unknown");
        break;
    }

    // 计算容量（2^capacity_code 字节）
    if (capacity_code >= 0x14 && capacity_code <= 0x20) {
        handle->info.capacity = 1 << capacity_code;
    } else {
        handle->info.capacity = 1024 * 1024; // 默认1MB
    }

    // 设置标准参数
    handle->info.page_size   = 256;   // 标准页大小
    handle->info.sector_size = 4096;  // 标准扇区大小 4KB
    handle->info.block_size  = 65536; // 标准块大小 64KB

    // 生成型号字符串
    snprintf(handle->info.model, sizeof(handle->info.model), "%02X%02X%02X",
             id_data[0], id_data[1], id_data[2]);

    return XY_NOR_OK;
}

/**
 * @brief 获取设备信息
 */
xy_nor_status_t xy_nor_get_info(xy_nor_handle_t *handle, xy_nor_info_t *info)
{
    if (handle == NULL || info == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    memcpy(info, &handle->info, sizeof(xy_nor_info_t));
    return XY_NOR_OK;
}

/**
 * @brief 写使能
 */
xy_nor_status_t xy_nor_write_enable(xy_nor_handle_t *handle)
{
    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_WRITE_ENABLE, 0, 0, NULL, 0, true);
}

/**
 * @brief 写禁止
 */
xy_nor_status_t xy_nor_write_disable(xy_nor_handle_t *handle)
{
    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_WRITE_DISABLE, 0, 0, NULL, 0, true);
}

/**
 * @brief 读取状态寄存器
 */
xy_nor_status_t xy_nor_read_status(xy_nor_handle_t *handle, uint8_t *status)
{
    if (handle == NULL || status == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_READ_STATUS, 0, 0, status, 1, false);
}

/**
 * @brief 写入状态寄存器
 */
xy_nor_status_t xy_nor_write_status(xy_nor_handle_t *handle, uint8_t status)
{
    xy_nor_status_t ret;

    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    // 写使能
    ret = xy_nor_write_enable(handle);
    if (ret != XY_NOR_OK) {
        return ret;
    }

    // 写入状态寄存器
    ret = xy_nor_send_command(
        handle, XY_NOR_CMD_WRITE_STATUS, 0, 0, &status, 1, true);
    if (ret != XY_NOR_OK) {
        return ret;
    }

    // 等待完成
    return xy_nor_wait_ready(handle);
}

/**
 * @brief 等待操作完成
 */
xy_nor_status_t xy_nor_wait_ready(xy_nor_handle_t *handle)
{
    uint32_t timeout_count = 0;
    uint8_t status         = 0;
    xy_nor_status_t ret;

    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    // 轮询状态寄存器
    while (timeout_count < handle->config.timeout_ms) {
        ret = xy_nor_read_status(handle, &status);
        if (ret != XY_NOR_OK) {
            return ret;
        }

        // 检查WIP位
        if ((status & XY_NOR_STATUS_WIP) == 0) {
            return XY_NOR_OK;
        }

        xy_nor_hw_delay_ms(1);
        timeout_count++;
    }

    return XY_NOR_TIMEOUT;
}

/**
 * @brief 读取数据
 */
xy_nor_status_t xy_nor_read(xy_nor_handle_t *handle, uint32_t address,
                            uint8_t *data, uint32_t size)
{
    if (handle == NULL || data == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    if (address + size > handle->info.capacity) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_READ_DATA, address, 3, data, size, false);
}

/**
 * @brief 快速读取数据
 */
xy_nor_status_t xy_nor_fast_read(xy_nor_handle_t *handle, uint32_t address,
                                 uint8_t *data, uint32_t size)
{
    if (handle == NULL || data == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    if (address + size > handle->info.capacity) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_FAST_READ, address, 3, data, size, false);
}

/**
 * @brief 四线模式读取数据
 */
xy_nor_status_t xy_nor_quad_read(xy_nor_handle_t *handle, uint32_t address,
                                 uint8_t *data, uint32_t size)
{
    if (handle == NULL || data == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    if (!handle->config.quad_enable) {
        return XY_NOR_ERROR; // 四线模式未使能
    }

    if (address + size > handle->info.capacity) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_QUAD_READ, address, 3, data, size, false);
}

/**
 * @brief 页编程
 */
xy_nor_status_t xy_nor_page_program(xy_nor_handle_t *handle, uint32_t address,
                                    const uint8_t *data, uint32_t size)
{
    xy_nor_status_t status;

    if (handle == NULL || data == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    if (size > handle->info.page_size) {
        return XY_NOR_INVALID_PARAM;
    }

    if (address + size > handle->info.capacity) {
        return XY_NOR_INVALID_PARAM;
    }

    // 写使能
    status = xy_nor_write_enable(handle);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 发送页编程命令
    status = xy_nor_send_command(handle, XY_NOR_CMD_PAGE_PROGRAM, address, 3,
                                 (uint8_t *)data, size, true);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 等待编程完成
    return xy_nor_wait_ready(handle);
}

/**
 * @brief 扇区擦除
 */
xy_nor_status_t xy_nor_sector_erase(xy_nor_handle_t *handle, uint32_t address)
{
    xy_nor_status_t status;

    if (handle == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    if (address >= handle->info.capacity) {
        return XY_NOR_INVALID_PARAM;
    }

    // 对齐检查
    if (address % handle->info.sector_size != 0) {
        return XY_NOR_INVALID_PARAM;
    }

    // 写使能
    status = xy_nor_write_enable(handle);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 发送扇区擦除命令
    status = xy_nor_send_command(
        handle, XY_NOR_CMD_SECTOR_ERASE, address, 3, NULL, 0, true);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 等待擦除完成
    return xy_nor_wait_ready(handle);
}

/**
 * @brief 块擦除
 */
xy_nor_status_t xy_nor_block_erase(xy_nor_handle_t *handle, uint32_t address)
{
    xy_nor_status_t status;

    if (handle == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    if (address >= handle->info.capacity) {
        return XY_NOR_INVALID_PARAM;
    }

    // 对齐检查
    if (address % handle->info.block_size != 0) {
        return XY_NOR_INVALID_PARAM;
    }

    // 写使能
    status = xy_nor_write_enable(handle);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 发送块擦除命令
    status = xy_nor_send_command(
        handle, XY_NOR_CMD_BLOCK_ERASE, address, 3, NULL, 0, true);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 等待擦除完成
    return xy_nor_wait_ready(handle);
}

/**
 * @brief 整片擦除
 */
xy_nor_status_t xy_nor_chip_erase(xy_nor_handle_t *handle)
{
    xy_nor_status_t status;

    if (handle == NULL || !handle->is_initialized) {
        return XY_NOR_INVALID_PARAM;
    }

    // 写使能
    status = xy_nor_write_enable(handle);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 发送整片擦除命令
    status =
        xy_nor_send_command(handle, XY_NOR_CMD_CHIP_ERASE, 0, 0, NULL, 0, true);
    if (status != XY_NOR_OK) {
        return status;
    }

    // 等待擦除完成（整片擦除时间较长）
    return xy_nor_wait_ready(handle);
}

/**
 * @brief 进入掉电模式
 */
xy_nor_status_t xy_nor_power_down(xy_nor_handle_t *handle)
{
    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_send_command(
        handle, XY_NOR_CMD_POWER_DOWN, 0, 0, NULL, 0, true);
}

/**
 * @brief 退出掉电模式
 */
xy_nor_status_t xy_nor_release_power_down(xy_nor_handle_t *handle)
{
    xy_nor_status_t status;

    if (handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    status = xy_nor_send_command(
        handle, XY_NOR_CMD_RELEASE_POWER_DOWN, 0, 0, NULL, 0, true);

    // 等待芯片唤醒
    xy_nor_hw_delay_ms(1);

    return status;
}

/**
 * @brief 内部命令发送函数
 */
static xy_nor_status_t xy_nor_send_command(xy_nor_handle_t *handle, uint8_t cmd,
                                           uint32_t addr, uint8_t addr_len,
                                           uint8_t *data, uint32_t data_len,
                                           bool is_write)
{
    if (handle == NULL || handle->hw_handle == NULL) {
        return XY_NOR_INVALID_PARAM;
    }

    return xy_nor_hw_command(
        handle->hw_handle, cmd, addr, addr_len, data, data_len, is_write);
}
