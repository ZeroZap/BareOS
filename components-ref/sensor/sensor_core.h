#ifndef __SENSOR_CORE_H__
#define __SENSOR_CORE_H__

#include "sensor_type.h"

/* 前置声明 */
struct sensor_device;

/* ==================== 回调函数类型 ==================== */
typedef void (*sensor_callback_t)(struct sensor_device *sensor,
                                  sensor_data_t *data, void *user_data);

/* ==================== 设备操作接口 ==================== */
typedef struct sensor_ops {
    /* 基础接口 (必须实现) */
    sensor_err_t (*init)(struct sensor_device *sensor);
    sensor_err_t (*deinit)(struct sensor_device *sensor);
    sensor_err_t (*read)(struct sensor_device *sensor, sensor_data_t *data);

    /* 可选接口 */
    sensor_err_t (*write)(struct sensor_device *sensor,
                          const sensor_data_t *data);
    sensor_err_t (*control)(struct sensor_device *sensor, int cmd, void *args);
    sensor_err_t (*config)(struct sensor_device *sensor,
                           sensor_config_type_t cfg, void *value);
    sensor_err_t (*enable)(struct sensor_device *sensor, bool enable);

#if SENSOR_ENABLE_FIFO
    /* FIFO接口 */
    sensor_err_t (*fifo_enable)(struct sensor_device *sensor, bool enable);
    sensor_err_t (*fifo_read)(struct sensor_device *sensor, sensor_data_t *data,
                              uint32_t count, uint32_t *read_count);
    sensor_err_t (*fifo_flush)(struct sensor_device *sensor);
    sensor_err_t (*fifo_get_count)(struct sensor_device *sensor,
                                   uint32_t *count);
#endif

#if SENSOR_ENABLE_INTERRUPT
    /* 中断接口 */
    sensor_err_t (*interrupt_enable)(struct sensor_device *sensor,
                                     uint32_t int_type, bool enable);
    sensor_err_t (*interrupt_clear)(struct sensor_device *sensor,
                                    uint32_t int_type);
#endif

#if SENSOR_ENABLE_CALIBRATION
    /* 校准接口 */
    sensor_err_t (*calibrate)(struct sensor_device *sensor,
                              sensor_calibration_type_t type);
    sensor_err_t (*get_calibration)(struct sensor_device *sensor,
                                    sensor_calibration_data_t *calib);
    sensor_err_t (*set_calibration)(struct sensor_device *sensor,
                                    const sensor_calibration_data_t *calib);
#endif

#if SENSOR_ENABLE_SELF_TEST
    /* 自测试接口 */
    sensor_err_t (*self_test)(struct sensor_device *sensor,
                              sensor_self_test_result_t *result);
#endif

#if SENSOR_ENABLE_POWER_MGMT
    /* 功耗管理接口 */
    sensor_err_t (*set_power_mode)(struct sensor_device *sensor,
                                   sensor_power_mode_t mode);
    sensor_err_t (*get_power_stats)(struct sensor_device *sensor,
                                    sensor_power_stats_t *stats);
#endif
} sensor_ops_t;

/* ==================== 设备信息 ==================== */
typedef struct sensor_info {
    char name[SENSOR_NAME_MAX_LEN];
    const char *vendor;
    const char *model;
    uint32_t version;
    sensor_type_t type;
    sensor_unit_t unit;
    int32_t range_max;
    int32_t range_min;
    uint32_t resolution;
    uint32_t max_odr;
#if SENSOR_ENABLE_FIFO
    uint32_t fifo_size;
#endif
    uint32_t flags;
} sensor_info_t;

/* 功能标志位 */
#define SENSOR_FLAG_FIFO_SUPPORT   (1 << 0)
#define SENSOR_FLAG_INT_SUPPORT    (1 << 1)
#define SENSOR_FLAG_DMA_SUPPORT    (1 << 2)
#define SENSOR_FLAG_CALIBRATION    (1 << 3)
#define SENSOR_FLAG_SELF_TEST      (1 << 4)
#define SENSOR_FLAG_LOW_POWER      (1 << 5)
#define SENSOR_FLAG_HIGH_PRECISION (1 << 6)

/* ==================== FIFO缓冲区 ==================== */
#if SENSOR_ENABLE_FIFO
typedef struct sensor_fifo {
    sensor_data_t *buffer;
    uint32_t size;
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    uint16_t watermark;
    sensor_fifo_mode_t mode;
    bool overflow;
} sensor_fifo_t;
#endif

/* ==================== 设备结构体 ==================== */
typedef struct sensor_device {
    /* 设备信息 */
    sensor_info_t info;
    uint8_t id;
    sensor_status_t status;

    /* 设备操作 */
    const sensor_ops_t *ops;
    void *bus;
    void *priv_data;

    /* 配置参数 */
    uint32_t odr;
    sensor_trigger_mode_t trigger_mode;

#if SENSOR_ENABLE_POWER_MGMT
    sensor_power_mode_t power_mode;
    sensor_power_stats_t power_stats;
    uint32_t last_active_time;
#endif

#if SENSOR_ENABLE_FIFO
    sensor_fifo_t *fifo;
#endif

#if SENSOR_ENABLE_INTERRUPT
    sensor_interrupt_config_t interrupt_cfg;
#endif

#if SENSOR_ENABLE_CALIBRATION
    sensor_calibration_data_t calibration;
#endif

#if SENSOR_ENABLE_FILTER
    sensor_filter_config_t filter_cfg;
    void *filter_state;
#endif

#if SENSOR_ENABLE_DMA
    sensor_dma_config_t dma_cfg;
#endif

#if SENSOR_ENABLE_THRESHOLD
    sensor_threshold_config_t threshold_cfg;
#endif

#if SENSOR_ENABLE_MOTION_DETECT
    sensor_motion_config_t motion_cfg;
#endif

    /* 回调与用户数据 */
    sensor_callback_t callback;
    void *user_data;

    /* 链表节点 */
    struct sensor_device *next;
} sensor_device_t;

/* ==================== 基础API ==================== */
sensor_err_t sensor_register(sensor_device_t *sensor);
sensor_err_t sensor_unregister(sensor_device_t *sensor);
sensor_device_t *sensor_find_by_name(const char *name);
sensor_device_t *sensor_find_by_type(sensor_type_t type);
sensor_err_t sensor_init(sensor_device_t *sensor);
sensor_err_t sensor_deinit(sensor_device_t *sensor);
sensor_err_t sensor_enable(sensor_device_t *sensor, bool enable);
sensor_err_t sensor_read(sensor_device_t *sensor, sensor_data_t *data);
sensor_err_t sensor_write(sensor_device_t *sensor, const sensor_data_t *data);
sensor_err_t sensor_config(sensor_device_t *sensor, sensor_config_type_t cfg,
                           void *value);
sensor_err_t sensor_control(sensor_device_t *sensor, int cmd, void *args);
sensor_err_t sensor_set_callback(sensor_device_t *sensor,
                                 sensor_callback_t callback, void *user_data);
const sensor_info_t *sensor_get_info(sensor_device_t *sensor);
sensor_status_t sensor_get_status(sensor_device_t *sensor);

/* ==================== FIFO API ==================== */
#if SENSOR_ENABLE_FIFO
sensor_err_t sensor_fifo_init(sensor_device_t *sensor, uint32_t size);
sensor_err_t sensor_fifo_deinit(sensor_device_t *sensor);
sensor_err_t sensor_fifo_enable(sensor_device_t *sensor, bool enable);
sensor_err_t sensor_fifo_read(sensor_device_t *sensor, sensor_data_t *data,
                              uint32_t count, uint32_t *read_count);
sensor_err_t sensor_fifo_flush(sensor_device_t *sensor);
sensor_err_t sensor_fifo_get_count(sensor_device_t *sensor, uint32_t *count);
sensor_err_t sensor_fifo_set_watermark(sensor_device_t *sensor,
                                       uint16_t watermark);
bool sensor_fifo_is_full(sensor_device_t *sensor);
bool sensor_fifo_is_empty(sensor_device_t *sensor);
sensor_err_t sensor_fifo_push(sensor_device_t *sensor,
                              const sensor_data_t *data);
#endif

/* ==================== 中断API ==================== */
#if SENSOR_ENABLE_INTERRUPT
sensor_err_t sensor_interrupt_init(sensor_device_t *sensor,
                                   sensor_interrupt_config_t *config);
sensor_err_t sensor_interrupt_enable(sensor_device_t *sensor, uint32_t int_type,
                                     bool enable);
sensor_err_t sensor_interrupt_clear(sensor_device_t *sensor, uint32_t int_type);
void sensor_interrupt_handler(sensor_device_t *sensor, uint32_t int_type);
#endif

/* ==================== 校准API ==================== */
#if SENSOR_ENABLE_CALIBRATION
sensor_err_t sensor_calibrate(sensor_device_t *sensor,
                              sensor_calibration_type_t type);
sensor_err_t sensor_calibrate_offset(sensor_device_t *sensor,
                                     uint32_t sample_count);
sensor_err_t sensor_get_calibration(sensor_device_t *sensor,
                                    sensor_calibration_data_t *calib);
sensor_err_t sensor_set_calibration(sensor_device_t *sensor,
                                    const sensor_calibration_data_t *calib);
sensor_err_t sensor_apply_calibration(sensor_device_t *sensor,
                                      sensor_data_t *data);
#if SENSOR_USE_FILE_SYSTEM
sensor_err_t sensor_load_calibration(sensor_device_t *sensor,
                                     const char *filename);
sensor_err_t sensor_save_calibration(sensor_device_t *sensor,
                                     const char *filename);
#endif
#endif

/* ==================== 滤波器API ==================== */
#if SENSOR_ENABLE_FILTER
sensor_err_t sensor_filter_init(sensor_device_t *sensor,
                                sensor_filter_config_t *config);
sensor_err_t sensor_filter_deinit(sensor_device_t *sensor);
sensor_err_t sensor_filter_enable(sensor_device_t *sensor, bool enable);
sensor_err_t sensor_filter_process(sensor_device_t *sensor,
                                   sensor_data_t *data);
sensor_err_t sensor_filter_reset(sensor_device_t *sensor);
#endif

/* ==================== DMA API ==================== */
#if SENSOR_ENABLE_DMA
sensor_err_t sensor_dma_init(sensor_device_t *sensor,
                             sensor_dma_config_t *config);
sensor_err_t sensor_dma_deinit(sensor_device_t *sensor);
sensor_err_t sensor_dma_start(sensor_device_t *sensor);
sensor_err_t sensor_dma_stop(sensor_device_t *sensor);
void sensor_dma_complete_callback(sensor_device_t *sensor);
#endif

/* ==================== 阈值检测API ==================== */
#if SENSOR_ENABLE_THRESHOLD
sensor_err_t sensor_threshold_init(sensor_device_t *sensor,
                                   sensor_threshold_config_t *config);
sensor_err_t sensor_threshold_enable(sensor_device_t *sensor, bool enable);
bool sensor_threshold_check(sensor_device_t *sensor, sensor_data_t *data);
#endif

/* ==================== 运动检测API ==================== */
#if SENSOR_ENABLE_MOTION_DETECT
sensor_err_t sensor_motion_detect_init(sensor_device_t *sensor,
                                       sensor_motion_config_t *config);
sensor_err_t sensor_motion_detect_enable(sensor_device_t *sensor, bool enable);
sensor_err_t sensor_motion_detect_process(sensor_device_t *sensor,
                                          sensor_data_t *data);
sensor_err_t sensor_freefall_detect_init(sensor_device_t *sensor,
                                         sensor_freefall_config_t *config);
sensor_err_t sensor_freefall_detect_process(sensor_device_t *sensor,
                                            sensor_data_t *data);
sensor_err_t sensor_tap_detect_init(sensor_device_t *sensor,
                                    sensor_tap_config_t *config);
sensor_err_t sensor_tap_detect_process(sensor_device_t *sensor,
                                       sensor_data_t *data);
#endif

/* ==================== 功耗管理API ==================== */
#if SENSOR_ENABLE_POWER_MGMT
sensor_err_t sensor_set_power_mode(sensor_device_t *sensor,
                                   sensor_power_mode_t mode);
sensor_power_mode_t sensor_get_power_mode(sensor_device_t *sensor);
sensor_err_t sensor_get_power_stats(sensor_device_t *sensor,
                                    sensor_power_stats_t *stats);
sensor_err_t sensor_reset_power_stats(sensor_device_t *sensor);
#endif

/* ==================== 自测试API ==================== */
#if SENSOR_ENABLE_SELF_TEST
sensor_err_t sensor_self_test(sensor_device_t *sensor,
                              sensor_self_test_result_t *result);
#endif

/* ==================== 传感器融合API ==================== */
#if SENSOR_ENABLE_FUSION
sensor_err_t sensor_fusion_init(sensor_fusion_config_t *config);
sensor_err_t sensor_fusion_deinit(sensor_fusion_config_t *config);
sensor_err_t sensor_fusion_update(sensor_fusion_config_t *config,
                                  sensor_data_t *output);
sensor_err_t sensor_fusion_reset(void);
#if SENSOR_USE_FLOAT
sensor_err_t sensor_fusion_get_quaternion(float *q0, float *q1, float *q2,
                                          float *q3);
#endif
#endif

#endif /* __SENSOR_CORE_H__ */