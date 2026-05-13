#ifndef XY_NOR_H
#define XY_NOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NOR Flash 状态定义 */
typedef enum {
    XY_NOR_OK = 0,
    XY_NOR_ERROR = 1,
    XY_NOR_BUSY = 2,
    XY_NOR_TIMEOUT = 3,
    XY_NOR_INVALID_PARAM = 4
} xy_nor_status_t;

/* NOR Flash 电气参数配置结构体 */
typedef struct {
    uint32_t clock_freq;        // 时钟频率 (Hz)
    uint8_t dummy_cycles;       // 虚拟时钟周期数
    uint8_t drive_strength;     // 驱动强度 (0-7)
    uint8_t slew_rate;          // 摆率控制 (0-3)
    bool quad_enable;           // 四线模式使能
    bool write_protection;      // 写保护使能
    uint32_t timeout_ms;        // 超时时间 (毫秒)
    uint8_t voltage_range;      // 电压范围 (0: 1.8V, 1: 3.3V)
} xy_nor_config_t;

/* NOR Flash 设备信息结构体 */
typedef struct {
    uint32_t jedec_id;          // JEDEC ID
    uint32_t capacity;          // 容量 (字节)
    uint32_t page_size;         // 页大小
    uint32_t sector_size;       // 扇区大小
    uint32_t block_size;        // 块大小
    char manufacturer[16];      // 制造商名称
    char model[32];            // 型号
} xy_nor_info_t;

/* NOR Flash 操作句柄 */
typedef struct {
    xy_nor_config_t config;     // 配置参数
    xy_nor_info_t info;        // 设备信息
    bool is_initialized;        // 初始化状态
    void *hw_handle;           // 硬件句柄
} xy_nor_handle_t;

/* 常用NOR Flash命令 */
#define XY_NOR_CMD_WRITE_ENABLE     0x06
#define XY_NOR_CMD_WRITE_DISABLE    0x04
#define XY_NOR_CMD_READ_STATUS      0x05
#define XY_NOR_CMD_WRITE_STATUS     0x01
#define XY_NOR_CMD_READ_DATA        0x03
#define XY_NOR_CMD_FAST_READ        0x0B
#define XY_NOR_CMD_PAGE_PROGRAM     0x02
#define XY_NOR_CMD_SECTOR_ERASE     0x20
#define XY_NOR_CMD_BLOCK_ERASE      0xD8
#define XY_NOR_CMD_CHIP_ERASE       0xC7
#define XY_NOR_CMD_POWER_DOWN       0xB9
#define XY_NOR_CMD_RELEASE_POWER_DOWN 0xAB
#define XY_NOR_CMD_READ_ID          0x9F
#define XY_NOR_CMD_QUAD_READ        0xEB

/* 状态寄存器位定义 */
#define XY_NOR_STATUS_WIP           (1 << 0)  // Write In Progress
#define XY_NOR_STATUS_WEL           (1 << 1)  // Write Enable Latch
#define XY_NOR_STATUS_BP0           (1 << 2)  // Block Protect 0
#define XY_NOR_STATUS_BP1           (1 << 3)  // Block Protect 1
#define XY_NOR_STATUS_BP2           (1 << 4)  // Block Protect 2
#define XY_NOR_STATUS_TB            (1 << 5)  // Top/Bottom Protect
#define XY_NOR_STATUS_SEC           (1 << 6)  // Sector Protect
#define XY_NOR_STATUS_SRP           (1 << 7)  // Status Register Protect

/* 默认配置参数 */
#define XY_NOR_DEFAULT_CLOCK_FREQ   25000000  // 25MHz
#define XY_NOR_DEFAULT_DUMMY_CYCLES 8
#define XY_NOR_DEFAULT_DRIVE_STRENGTH 4
#define XY_NOR_DEFAULT_SLEW_RATE    2
#define XY_NOR_DEFAULT_TIMEOUT_MS   5000

/* 函数声明 */

/**
 * @brief 获取默认配置参数
 * @param config 配置参数结构体指针
 */
void xy_nor_get_default_config(xy_nor_config_t *config);

/**
 * @brief 初始化NOR Flash驱动
 * @param handle NOR Flash句柄
 * @param config 配置参数
 * @return 操作状态
 */
xy_nor_status_t xy_nor_init(xy_nor_handle_t *handle, const xy_nor_config_t *config);

/**
 * @brief 去初始化NOR Flash驱动
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_deinit(xy_nor_handle_t *handle);

/**
 * @brief 读取设备信息
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_read_info(xy_nor_handle_t *handle);

/**
 * @brief 读取数据
 * @param handle NOR Flash句柄
 * @param address 读取地址
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return 操作状态
 */
xy_nor_status_t xy_nor_read(xy_nor_handle_t *handle, uint32_t address, uint8_t *data, uint32_t size);

/**
 * @brief 快速读取数据
 * @param handle NOR Flash句柄
 * @param address 读取地址
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return 操作状态
 */
xy_nor_status_t xy_nor_fast_read(xy_nor_handle_t *handle, uint32_t address, uint8_t *data, uint32_t size);

/**
 * @brief 四线模式读取数据
 * @param handle NOR Flash句柄
 * @param address 读取地址
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return 操作状态
 */
xy_nor_status_t xy_nor_quad_read(xy_nor_handle_t *handle, uint32_t address, uint8_t *data, uint32_t size);

/**
 * @brief 页编程
 * @param handle NOR Flash句柄
 * @param address 写入地址
 * @param data 数据缓冲区
 * @param size 数据大小
 * @return 操作状态
 */
xy_nor_status_t xy_nor_page_program(xy_nor_handle_t *handle, uint32_t address, const uint8_t *data, uint32_t size);

/**
 * @brief 扇区擦除
 * @param handle NOR Flash句柄
 * @param address 扇区地址
 * @return 操作状态
 */
xy_nor_status_t xy_nor_sector_erase(xy_nor_handle_t *handle, uint32_t address);

/**
 * @brief 块擦除
 * @param handle NOR Flash句柄
 * @param address 块地址
 * @return 操作状态
 */
xy_nor_status_t xy_nor_block_erase(xy_nor_handle_t *handle, uint32_t address);

/**
 * @brief 整片擦除
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_chip_erase(xy_nor_handle_t *handle);

/**
 * @brief 写使能
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_write_enable(xy_nor_handle_t *handle);

/**
 * @brief 写禁止
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_write_disable(xy_nor_handle_t *handle);

/**
 * @brief 读取状态寄存器
 * @param handle NOR Flash句柄
 * @param status 状态值指针
 * @return 操作状态
 */
xy_nor_status_t xy_nor_read_status(xy_nor_handle_t *handle, uint8_t *status);

/**
 * @brief 写入状态寄存器
 * @param handle NOR Flash句柄
 * @param status 状态值
 * @return 操作状态
 */
xy_nor_status_t xy_nor_write_status(xy_nor_handle_t *handle, uint8_t status);

/**
 * @brief 等待操作完成
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_wait_ready(xy_nor_handle_t *handle);

/**
 * @brief 进入掉电模式
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_power_down(xy_nor_handle_t *handle);

/**
 * @brief 退出掉电模式
 * @param handle NOR Flash句柄
 * @return 操作状态
 */
xy_nor_status_t xy_nor_release_power_down(xy_nor_handle_t *handle);

/**
 * @brief 配置电气参数
 * @param handle NOR Flash句柄
 * @param config 配置参数
 * @return 操作状态
 */
xy_nor_status_t xy_nor_configure_electrical_params(xy_nor_handle_t *handle, const xy_nor_config_t *config);

/**
 * @brief 获取设备信息
 * @param handle NOR Flash句柄
 * @param info 设备信息指针
 * @return 操作状态
 */
xy_nor_status_t xy_nor_get_info(xy_nor_handle_t *handle, xy_nor_info_t *info);

/* 硬件抽象层接口 - 需要用户实现 */

/**
 * @brief 硬件初始化
 * @param config 配置参数
 * @return 硬件句柄
 */
void* xy_nor_hw_init(const xy_nor_config_t *config);

/**
 * @brief 硬件去初始化
 * @param hw_handle 硬件句柄
 */
void xy_nor_hw_deinit(void *hw_handle);

/**
 * @brief 发送命令
 * @param hw_handle 硬件句柄
 * @param cmd 命令
 * @param addr 地址
 * @param addr_len 地址长度
 * @param data 数据
 * @param data_len 数据长度
 * @param is_write 是否为写操作
 * @return 操作状态
 */
xy_nor_status_t xy_nor_hw_command(void *hw_handle, uint8_t cmd, uint32_t addr, 
                                  uint8_t addr_len, uint8_t *data, uint32_t data_len, bool is_write);

/**
 * @brief 延时函数
 * @param ms 延时毫秒数
 */
void xy_nor_hw_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* XY_NOR_H */
