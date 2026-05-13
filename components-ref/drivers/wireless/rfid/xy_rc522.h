/**
 * @file xy_rc522.h
 * @brief MFRC522 RFID Reader Driver
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 支持 MFRC522/RC522 RFID 读卡器 (SPI 接口)
 */

#ifndef XY_RC522_H
#define XY_RC522_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== RC522 Device Structure ==================== */

/**
 * @brief RC522 设备结构
 */
typedef struct {
    xy_device_t base;              /**< 设备基类 */
    void *spi_handle;              /**< SPI 句柄 */
    void *rst_gpio;                /**< 复位 GPIO */
    uint8_t firmware_version;      /**< 固件版本 */
    bool initialized;              /**< 初始化标志 */
} xy_rc522_t;

/* ==================== MIFARE Card Types ==================== */

/**
 * @brief MIFARE 卡类型
 */
typedef enum {
    XY_RC522_CARD_UNKNOWN = 0,
    XY_RC522_CARD_MIFARE_1K,       /**< MIFARE Classic 1K */
    XY_RC522_CARD_MIFARE_4K,       /**< MIFARE Classic 4K */
    XY_RC522_CARD_MIFARE_ULTRA,    /**< MIFARE Ultralight */
    XY_RC522_CARD_MIFARE_MINI,     /**< MIFARE Mini */
} xy_rc522_card_type_t;

/* ==================== UID Structure ==================== */

/**
 * @brief UID 结构
 */
typedef struct {
    uint8_t uid[10];               /**< UID 数据 */
    uint8_t uid_len;               /**< UID 长度 */
    xy_rc522_card_type_t card_type;/**< 卡类型 */
} xy_rc522_uid_t;

/* ==================== RC522 API ==================== */

/**
 * @brief 初始化 RC522
 * @param dev RC522 设备句柄
 * @param spi_handle SPI 句柄
 * @param rst_gpio 复位 GPIO
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_init(xy_rc522_t *dev, void *spi_handle, void *rst_gpio);

/**
 * @brief 反初始化 RC522
 * @param dev RC522 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_deinit(xy_rc522_t *dev);

/**
 * @brief 复位 RC522
 * @param dev RC522 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_reset(xy_rc522_t *dev);

/**
 * @brief 读取固件版本
 * @param dev RC522 设备句柄
 * @param version 版本号 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_get_version(xy_rc522_t *dev, uint8_t *version);

/**
 * @brief 检测是否有卡
 * @param dev RC522 设备句柄
 * @return XY_DEVICE_OK 有卡，XY_DEVICE_NOT_FOUND 无卡
 */
int xy_rc522_detect_card(xy_rc522_t *dev);

/**
 * @brief 选择卡
 * @param dev RC522 设备句柄
 * @param uid UID 结构 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_select_card(xy_rc522_t *dev, xy_rc522_uid_t *uid);

/**
 * @brief 读取 MIFARE 卡数据块
 * @param dev RC522 设备句柄
 * @param block_addr 块地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_mifare_read(xy_rc522_t *dev, uint8_t block_addr, 
                         uint8_t *data, uint8_t length);

/**
 * @brief 写入 MIFARE 卡数据块
 * @param dev RC522 设备句柄
 * @param block_addr 块地址
 * @param data 数据缓冲区
 * @param length 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_mifare_write(xy_rc522_t *dev, uint8_t block_addr, 
                          const uint8_t *data, uint8_t length);

/**
 * @brief 进入空闲模式
 * @param dev RC522 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_idle(xy_rc522_t *dev);

/**
 * @brief 进入掉电模式
 * @param dev RC522 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_rc522_powerdown(xy_rc522_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_RC522_H */
