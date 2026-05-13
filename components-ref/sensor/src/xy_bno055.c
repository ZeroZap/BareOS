/**
 * @file xy_bno055.c
 * @brief Bosch BNO055 9-Axis Smart IMU Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-18
 * 
 * @note BNO055 是内置传感器融合算法的 9 轴智能 IMU
 * 
 * 主要特性:
 * - 3 轴加速度计 + 3 轴陀螺仪 + 3 轴磁力计
 * - 内置 ARM Cortex-M0 处理器运行传感器融合算法
 * - 输出：四元数/欧拉角/线性加速度/重力向量
 * - 免校准：内置自动校准算法
 * - I2C/UART 接口
 * 
 * 使用示例:
 * @code
 * xy_bno055_t bno055;
 * xy_bno055_init(&bno055, i2c_handle, 0x28, false);
 * 
 * // 设置 NDOF 模式 (完全 9 轴融合)
 * xy_bno055_set_mode(&bno055, BNO055_MODE_NDOF);
 * 
 * // 读取姿态数据
 * bno055_data_t data;
 * xy_bno055_get_data(&bno055, &data);
 * 
 * // 欧拉角
 * printf("Heading: %.2f, Roll: %.2f, Pitch: %.2f\n",
 *        data.euler.heading, data.euler.roll, data.euler.pitch);
 * @endcode
 */

#include "xy_bno055.h"
#include "xy_i2c.h"
#include "xy_uart.h"
#include "xy_log.h"
#include <string.h>

#define XY_LOG_TAG "BNO055"

/* ==================== 常量定义 ==================== */

/** 重力加速度 (m/s²) */
#define GRAVITY_MSS2               (9.80665f)

/** 角度转弧度 */
#define DEG_TO_RAD                 (0.017453292519943295f)

/** 弧度转角度 */
#define RAD_TO_DEG                 (57.29577951308232f)

/** 磁场单位转换 (1 mG = 0.1 μT) */
#define MG_TO_UT                   (0.1f)

/** 初始化超时 (ms) */
#define BNO055_INIT_TIMEOUT        (1000)

/** 模式切换延迟 (ms) */
#define BNO055_MODE_DELAY          (20)

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 延迟函数
 */
static void delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    for (uint32_t j = 0; j < ms; j++) {
        for (i = 0; i < 10000; i++);
    }
}

/**
 * @brief 总线读取
 */
static int bus_read(xy_bno055_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev->is_uart) {
        /* UART 模式：需要构造命令帧 */
        /* 简化实现，实际需要根据 UART 协议 */
        return XY_DEVICE_ENOSYS;
    } else {
        /* I2C 模式 */
        return xy_i2c_master_transmit((xy_i2c_t*)dev->bus_handle, 
                                      dev->bus_addr, &reg, 1, buf, len, 100);
    }
}

/**
 * @brief 总线写入
 */
static int bus_write(xy_bno055_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if (dev->is_uart) {
        /* UART 模式 */
        return XY_DEVICE_ENOSYS;
    } else {
        /* I2C 模式 */
        return xy_i2c_master_transmit((xy_i2c_t*)dev->bus_handle, 
                                      dev->bus_addr, &reg, 1, buf, len, 100);
    }
}

/**
 * @brief 切换到配置模式
 */
static int switch_to_config_mode(xy_bno055_t *dev)
{
    bno055_mode_t current_mode;
    int ret = xy_bno055_get_mode(dev, &current_mode);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    if (current_mode != BNO055_MODE_CONFIG) {
        uint8_t mode = BNO055_MODE_CONFIG;
        ret = bus_write(dev, BNO055_REG_OPR_MODE, &mode, 1);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
        delay_ms(BNO055_MODE_DELAY);
    }

    return XY_DEVICE_OK;
}

/* ==================== 公共 API 实现 ==================== */

int xy_bno055_init(xy_bno055_t *dev, void *bus_handle, uint8_t bus_addr, bool is_uart)
{
    if (!dev || !bus_handle) {
        XY_LOG_ERROR("Invalid parameters");
        return XY_DEVICE_EINVAL;
    }

    memset(dev, 0, sizeof(xy_bno055_t));
    dev->bus_handle = bus_handle;
    dev->bus_addr = bus_addr;
    dev->is_uart = is_uart;
    dev->initialized = false;
    dev->mode = BNO055_MODE_CONFIG;

    XY_LOG_INFO("Initializing BNO055 on %s (addr: 0x%02X)...", 
                is_uart ? "UART" : "I2C", bus_addr);

    /* 软复位 */
    int ret = xy_bno055_reset(dev);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Reset failed: %d", ret);
        return ret;
    }

    /* 等待复位完成 (BNO055 需要较长时间) */
    delay_ms(650);

    /* 读取芯片 ID */
    uint8_t chip_id = 0;
    ret = xy_bno055_get_chip_id(dev, &chip_id);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to read chip ID: %d", ret);
        return ret;
    }

    if (chip_id != BNO055_CHIP_ID) {
        XY_LOG_ERROR("Invalid chip ID: 0x%02X (expected 0xA0)", chip_id);
        return XY_DEVICE_ENODEV;
    }

    XY_LOG_INFO("BNO055 found, chip ID: 0x%02X", chip_id);

    /* 读取固件版本 */
    ret = xy_bno055_get_sw_version(dev, &dev->sw_version);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to read firmware version: %d", ret);
        return ret;
    }

    XY_LOG_INFO("Firmware version: 0x%04X", dev->sw_version);

    /* 检查固件版本 */
    if (dev->sw_version < BNO055_SW_REV_MIN) {
        XY_LOG_WARNING("Firmware version too old: 0x%04X (min: 0x%04X)", 
                       dev->sw_version, BNO055_SW_REV_MIN);
    }

    /* 切换到配置模式 */
    ret = switch_to_config_mode(dev);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to switch to config mode: %d", ret);
        return ret;
    }

    /* 设置为内部晶振 */
    uint8_t sys_trigger = 0x00;
    ret = bus_write(dev, BNO055_REG_SYS_TRIGGER, &sys_trigger, 1);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to configure system trigger: %d", ret);
        return ret;
    }

    /* 设置单位为：角度=度，温度=摄氏度，加速度=m/s²，磁场=μT */
    dev->unit_flags = BNO055_UNIT_DEG | BNO055_UNIT_CELSIUS | 
                      BNO055_UNIT_EULER | BNO055_UNIT_MS2 | BNO055_UNIT_UT;
    ret = xy_bno055_set_units(dev, dev->unit_flags);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set units: %d", ret);
        return ret;
    }

    /* 设置 NDOF 模式 (完全 9 轴传感器融合) */
    ret = xy_bno055_set_mode(dev, BNO055_MODE_NDOF);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set NDOF mode: %d", ret);
        return ret;
    }

    /* 等待传感器融合启动 */
    delay_ms(100);

    dev->initialized = true;
    XY_LOG_INFO("BNO055 initialized successfully (NDOF mode)");

    return XY_DEVICE_OK;
}

int xy_bno055_deinit(xy_bno055_t *dev)
{
    if (!dev) {
        return XY_DEVICE_EINVAL;
    }

    if (dev->initialized) {
        /* 进入挂起模式 */
        xy_bno055_sleep(dev);
        dev->initialized = false;
        XY_LOG_INFO("BNO055 deinitialized");
    }

    return XY_DEVICE_OK;
}

int xy_bno055_read_regs(xy_bno055_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (!dev || !buf || !len || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return bus_read(dev, reg, buf, len);
}

int xy_bno055_write_regs(xy_bno055_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if (!dev || !buf || !len || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return bus_write(dev, reg, buf, len);
}

int xy_bno055_get_chip_id(xy_bno055_t *dev, uint8_t *chip_id)
{
    if (!dev || !chip_id) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bno055_read_regs(dev, BNO055_REG_CHIP_ID, chip_id, 1);
}

int xy_bno055_get_sw_version(xy_bno055_t *dev, uint16_t *version)
{
    if (!dev || !version) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t buf[2];
    int ret = xy_bno055_read_regs(dev, BNO055_REG_SW_REV_ID_LSB, buf, 2);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    *version = (uint16_t)((buf[1] << 8) | buf[0]);
    return XY_DEVICE_OK;
}

int xy_bno055_set_mode(xy_bno055_t *dev, bno055_mode_t mode)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    /* 先切换到配置模式 (如果需要) */
    if (mode == BNO055_MODE_CONFIG) {
        int ret = switch_to_config_mode(dev);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
    } else {
        /* 从当前模式切换到目标模式 */
        uint8_t mode_val = (uint8_t)mode;
        int ret = bus_write(dev, BNO055_REG_OPR_MODE, &mode_val, 1);
        if (ret != XY_DEVICE_OK) {
            XY_LOG_ERROR("Failed to set mode: %d", ret);
            return ret;
        }
        delay_ms(BNO055_MODE_DELAY);
    }

    dev->mode = mode;
    XY_LOG_INFO("BNO055 mode set to: %d", mode);
    return XY_DEVICE_OK;
}

int xy_bno055_get_mode(xy_bno055_t *dev, bno055_mode_t *mode)
{
    if (!dev || !mode) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t mode_val = 0;
    int ret = xy_bno055_read_regs(dev, BNO055_REG_OPR_MODE, &mode_val, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    *mode = (bno055_mode_t)(mode_val & 0x0F);
    return XY_DEVICE_OK;
}

int xy_bno055_set_power_mode(xy_bno055_t *dev, bno055_pwr_t pwr)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t pwr_val = (uint8_t)pwr;
    return bus_write(dev, BNO055_REG_PWR_MODE, &pwr_val, 1);
}

int xy_bno055_set_units(xy_bno055_t *dev, uint8_t unit_flags)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return bus_write(dev, BNO055_REG_UNIT_SEL, &unit_flags, 1);
}

int xy_bno055_reset(xy_bno055_t *dev)
{
    if (!dev) {
        return XY_DEVICE_EINVAL;
    }

    /* 写入 0x20 到 SYS_TRIGGER 触发软复位 */
    uint8_t trigger = 0x20;
    int ret = bus_write(dev, BNO055_REG_SYS_TRIGGER, &trigger, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    dev->initialized = false;
    return XY_DEVICE_OK;
}

int xy_bno055_get_calib_status(xy_bno055_t *dev, bno055_calib_t *calib)
{
    if (!dev || !calib || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t calib_val = 0;
    int ret = xy_bno055_read_regs(dev, BNO055_REG_CALIB_STAT, &calib_val, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 解析校准状态 */
    calib->sys = (calib_val >> 6) & 0x03;
    calib->gyro = (calib_val >> 4) & 0x03;
    calib->acc = (calib_val >> 2) & 0x03;
    calib->mag = calib_val & 0x03;

    return XY_DEVICE_OK;
}

int xy_bno055_get_quaternion(xy_bno055_t *dev, bno055_quaternion_t *quat)
{
    if (!dev || !quat || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t buf[8];
    int ret = xy_bno055_read_regs(dev, BNO055_REG_QUA_DATA_W_LSB, buf, 8);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 解析四元数 (每个分量 16 位，比例因子 1/16384) */
    int16_t w = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t x = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t y = (int16_t)((buf[5] << 8) | buf[4]);
    int16_t z = (int16_t)((buf[7] << 8) | buf[6]);

    quat->w = (float)w / 16384.0f;
    quat->x = (float)x / 16384.0f;
    quat->y = (float)y / 16384.0f;
    quat->z = (float)z / 16384.0f;

    return XY_DEVICE_OK;
}

int xy_bno055_get_euler(xy_bno055_t *dev, bno055_euler_t *euler)
{
    if (!dev || !euler || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t buf[6];
    int ret = xy_bno055_read_regs(dev, BNO055_REG_EUL_HEADING_LSB, buf, 6);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 解析欧拉角 (每个分量 16 位，比例因子 1/16) */
    int16_t heading = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t roll = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t pitch = (int16_t)((buf[5] << 8) | buf[4]);

    euler->heading = (float)heading / 16.0f;
    euler->roll = (float)roll / 16.0f;
    euler->pitch = (float)pitch / 16.0f;

    return XY_DEVICE_OK;
}

int xy_bno055_get_data(xy_bno055_t *dev, bno055_data_t *data)
{
    if (!dev || !data || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    int ret;

    /* 读取欧拉角 */
    ret = xy_bno055_get_euler(dev, &data->euler);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 读取四元数 */
    ret = xy_bno055_get_quaternion(dev, &data->quat);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 读取加速度数据 (0x08-0x0D, 6 字节) */
    uint8_t acc_buf[6];
    ret = xy_bno055_read_regs(dev, BNO055_REG_ACC_DATA_X_LSB, acc_buf, 6);
    if (ret == XY_DEVICE_OK) {
        int16_t acc_x = (int16_t)((acc_buf[1] << 8) | acc_buf[0]);
        int16_t acc_y = (int16_t)((acc_buf[3] << 8) | acc_buf[2]);
        int16_t acc_z = (int16_t)((acc_buf[5] << 8) | acc_buf[4]);
        
        /* 转换为 m/s² (比例因子 1/100) */
        data->acc_x = (float)acc_x / 100.0f;
        data->acc_y = (float)acc_y / 100.0f;
        data->acc_z = (float)acc_z / 100.0f;
    }

    /* 读取线性加速度 (0x28-0x2D, 6 字节) */
    uint8_t lia_buf[6];
    ret = xy_bno055_read_regs(dev, BNO055_REG_LIA_DATA_X_LSB, lia_buf, 6);
    if (ret == XY_DEVICE_OK) {
        int16_t lia_x = (int16_t)((lia_buf[1] << 8) | lia_buf[0]);
        int16_t lia_y = (int16_t)((lia_buf[3] << 8) | lia_buf[2]);
        int16_t lia_z = (int16_t)((lia_buf[5] << 8) | lia_buf[4]);
        
        data->linear_acc_x = (float)lia_x / 100.0f;
        data->linear_acc_y = (float)lia_y / 100.0f;
        data->linear_acc_z = (float)lia_z / 100.0f;
    }

    /* 读取重力向量 (0x2E-0x33, 6 字节) */
    uint8_t grv_buf[6];
    ret = xy_bno055_read_regs(dev, BNO055_REG_GRV_DATA_X_LSB, grv_buf, 6);
    if (ret == XY_DEVICE_OK) {
        int16_t grv_x = (int16_t)((grv_buf[1] << 8) | grv_buf[0]);
        int16_t grv_y = (int16_t)((grv_buf[3] << 8) | grv_buf[2]);
        int16_t grv_z = (int16_t)((grv_buf[5] << 8) | grv_buf[4]);
        
        data->gravity_x = (float)grv_x / 100.0f;
        data->gravity_y = (float)grv_y / 100.0f;
        data->gravity_z = (float)grv_z / 100.0f;
    }

    /* 读取磁场数据 (0x0E-0x13, 6 字节) */
    uint8_t mag_buf[6];
    ret = xy_bno055_read_regs(dev, BNO055_REG_MAG_DATA_X_LSB, mag_buf, 6);
    if (ret == XY_DEVICE_OK) {
        int16_t mag_x = (int16_t)((mag_buf[1] << 8) | mag_buf[0]);
        int16_t mag_y = (int16_t)((mag_buf[3] << 8) | mag_buf[2]);
        int16_t mag_z = (int16_t)((mag_buf[5] << 8) | mag_buf[4]);
        
        /* 转换为 μT (比例因子 1/16) */
        data->mag_x = (float)mag_x / 16.0f;
        data->mag_y = (float)mag_y / 16.0f;
        data->mag_z = (float)mag_z / 16.0f;
    }

    /* 读取陀螺仪数据 (0x14-0x19, 6 字节) */
    uint8_t gyr_buf[6];
    ret = xy_bno055_read_regs(dev, BNO055_REG_GYR_DATA_X_LSB, gyr_buf, 6);
    if (ret == XY_DEVICE_OK) {
        int16_t gyr_x = (int16_t)((gyr_buf[1] << 8) | gyr_buf[0]);
        int16_t gyr_y = (int16_t)((gyr_buf[3] << 8) | gyr_buf[2]);
        int16_t gyr_z = (int16_t)((gyr_buf[5] << 8) | gyr_buf[4]);
        
        /* 转换为 rad/s (比例因子 1/16 °/s) */
        data->gyr_x = (float)gyr_x / 16.0f * DEG_TO_RAD;
        data->gyr_y = (float)gyr_y / 16.0f * DEG_TO_RAD;
        data->gyr_z = (float)gyr_z / 16.0f * DEG_TO_RAD;
    }

    /* 读取温度 */
    int8_t temp = 0;
    ret = xy_bno055_read_regs(dev, BNO055_REG_TEMP, (uint8_t*)&temp, 1);
    if (ret == XY_DEVICE_OK) {
        data->temperature = (float)temp;
    }

    /* 读取校准状态 */
    ret = xy_bno055_get_calib_status(dev, &data->calib);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    return XY_DEVICE_OK;
}

int xy_bno055_get_raw_data(xy_bno055_t *dev, bno055_raw_data_t *raw)
{
    if (!dev || !raw || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t buf[45];  /* 所有传感器数据 */
    int ret = xy_bno055_read_regs(dev, BNO055_REG_ACC_DATA_X_LSB, buf, 45);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 解析加速度 */
    raw->acc_x = (int16_t)((buf[1] << 8) | buf[0]);
    raw->acc_y = (int16_t)((buf[3] << 8) | buf[2]);
    raw->acc_z = (int16_t)((buf[5] << 8) | buf[4]);

    /* 解析磁场 */
    raw->mag_x = (int16_t)((buf[7] << 8) | buf[6]);
    raw->mag_y = (int16_t)((buf[9] << 8) | buf[8]);
    raw->mag_z = (int16_t)((buf[11] << 8) | buf[10]);

    /* 解析陀螺仪 */
    raw->gyr_x = (int16_t)((buf[13] << 8) | buf[12]);
    raw->gyr_y = (int16_t)((buf[15] << 8) | buf[14]);
    raw->gyr_z = (int16_t)((buf[17] << 8) | buf[16]);

    /* 温度 */
    raw->temp = (int8_t)buf[45];

    return XY_DEVICE_OK;
}

int xy_bno055_get_sys_status(xy_bno055_t *dev, uint8_t *status)
{
    if (!dev || !status || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bno055_read_regs(dev, BNO055_REG_SYS_STATUS, status, 1);
}

int xy_bno055_get_self_test(xy_bno055_t *dev, uint8_t *result)
{
    if (!dev || !result || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bno055_read_regs(dev, BNO055_REG_ST_RESULT, result, 1);
}

int xy_bno055_set_axis_remap(xy_bno055_t *dev, uint8_t config, uint8_t sign)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    /* 先切换到配置模式 */
    int ret = switch_to_config_mode(dev);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 设置重映射配置 */
    ret = bus_write(dev, BNO055_REG_AXIS_MAP_CONFIG, &config, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 设置符号 */
    ret = bus_write(dev, BNO055_REG_AXIS_MAP_SIGN, &sign, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 切换回 NDOF 模式 */
    return xy_bno055_set_mode(dev, BNO055_MODE_NDOF);
}

int xy_bno055_sleep(xy_bno055_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bno055_set_power_mode(dev, BNO055_PWR_SUSPEND);
}

int xy_bno055_wakeup(xy_bno055_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bno055_set_power_mode(dev, BNO055_PWR_NORMAL);
}
