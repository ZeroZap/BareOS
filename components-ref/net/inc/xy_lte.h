/**
 * @file xy_lte.h
 * @brief LTE/4G Cat.1 Module Driver
 * @version 1.0.0
 * @date 2026-03-13
 * 
 * 支持模块:
 * - 移远 EC100Y/EC200Y
 * - 广和通 L610/L630
 * - 合宙 Air780E
 */

#ifndef XY_LTE_H
#define XY_LTE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief LTE 错误码
 */
#define XY_LTE_OK               0
#define XY_LTE_ERROR            (-1)
#define XY_LTE_INVALID_PARAM    (-2)
#define XY_LTE_TIMEOUT          (-3)
#define XY_LTE_NO_NETWORK       (-4)
#define XY_LTE_NO_SIM           (-5)
#define XY_LTE_PIN_REQUIRED     (-6)
#define XY_LTE_ATTACH_FAILED    (-7)
#define XY_LTE_PDP_FAILED       (-8)

/**
 * @brief LTE 网络类型
 */
typedef enum {
    XY_LTE_NET_NONE = 0,
    XY_LTE_NET_2G,        /* GSM/GPRS/EDGE */
    XY_LTE_NET_3G,        /* WCDMA/TD-SCDMA */
    XY_LTE_NET_4G,        /* LTE-TDD/LTE-FDD */
    XY_LTE_NET_NB_IOT,    /* NB-IoT */
    XY_LTE_NET_CATM1,     /* LTE Cat-M1 */
} xy_lte_net_type_t;

/**
 * @brief LTE 信号质量
 */
typedef struct {
    int rssi;             /* 信号强度 (-113 ~ -51 dBm) */
    int ber;              /* 误码率 (0-7) */
    int rsrp;             /* 参考信号接收功率 (4G) */
    int rsrq;             /* 参考信号接收质量 (4G) */
    int sinr;             /* 信噪比 (4G) */
} xy_lte_signal_t;

/**
 * @brief LTE 网络信息
 */
typedef struct {
    xy_lte_net_type_t net_type;
    char mcc[4];          /* 移动国家码 */
    char mnc[3];          /* 移动网络码 */
    uint16_t lac;         /* 位置区码 */
    uint32_t cell_id;     /* 小区 ID */
    int earfcn;           /* 频点 (4G) */
} xy_lte_network_info_t;

/**
 * @brief LTE SIM 信息
 */
typedef struct {
    char iccid[21];       /* ICCID */
    char imsi[16];        /* IMSI */
    char msisdn[21];      /* 手机号 */
} xy_lte_sim_info_t;

/**
 * @brief LTE PDP 上下文配置
 */
typedef struct {
    uint8_t cid;          /* 上下文 ID (1-16) */
    char apn[64];         /* APN 名称 */
    char username[32];    /* 用户名 */
    char password[32];    /* 密码 */
    uint8_t auth_type;    /* 认证类型：0=无，1=PAP，2=CHAP */
} xy_lte_pdp_context_t;

/**
 * @brief LTE 模块句柄
 */
typedef struct {
    void *uart_handle;          /* UART 句柄 */
    uint32_t baudrate;          /* 波特率 */
    bool initialized;           /* 初始化标志 */
    bool attached;              /* 附着网络 */
    bool pdp_active;            /* PDP 激活 */
    xy_lte_net_type_t net_type; /* 当前网络类型 */
    xy_lte_signal_t signal;     /* 信号质量 */
    xy_lte_pdp_context_t pdp;   /* PDP 配置 */
} xy_lte_t;

/**
 * @brief LTE 回调函数
 */
typedef void (*xy_lte_urc_callback_t)(const char *urc);
typedef void (*xy_lte_recv_callback_t)(uint8_t *data, size_t len);

/**
 * @brief 初始化 LTE 模块
 * @param lte LTE 句柄
 * @param uart_handle UART 句柄
 * @param baudrate 波特率
 * @return XY_LTE_OK 成功
 */
int xy_lte_init(xy_lte_t *lte, void *uart_handle, uint32_t baudrate);

/**
 * @brief 反初始化 LTE 模块
 */
int xy_lte_deinit(xy_lte_t *lte);

/**
 * @brief 检查模块通信
 * @return XY_LTE_OK 模块正常
 */
int xy_lte_check(xy_lte_t *lte);

/**
 * @brief 获取模块信息
 */
int xy_lte_get_module_info(xy_lte_t *lte, char *manufacturer, char *model, char *revision);

/**
 * @brief 获取 SIM 信息
 */
int xy_lte_get_sim_info(xy_lte_t *lte, xy_lte_sim_info_t *info);

/**
 * @brief 检查 SIM 卡状态
 * @return 0=无卡，1=有卡
 */
int xy_lte_check_sim(xy_lte_t *lte);

/**
 * @brief 输入 PIN 码
 */
int xy_lte_enter_pin(xy_lte_t *lte, const char *pin);

/**
 * @brief 获取信号质量
 */
int xy_lte_get_signal(xy_lte_t *lte, xy_lte_signal_t *signal);

/**
 * @brief 获取网络信息
 */
int xy_lte_get_network_info(xy_lte_t *lte, xy_lte_network_info_t *info);

/**
 * @brief 附着网络
 */
int xy_lte_attach(xy_lte_t *lte);

/**
 * @brief 分离网络
 */
int xy_lte_detach(xy_lte_t *lte);

/**
 * @brief 检查附着状态
 * @return 0=未附着，1=已附着
 */
int xy_lte_is_attached(xy_lte_t *lte);

/**
 * @brief 配置 PDP 上下文
 */
int xy_lte_set_pdp_context(xy_lte_t *lte, xy_lte_pdp_context_t *ctx);

/**
 * @brief 激活 PDP 上下文
 */
int xy_lte_activate_pdp(xy_lte_t *lte, uint8_t cid);

/**
 * @brief 去激活 PDP 上下文
 */
int xy_lte_deactivate_pdp(xy_lte_t *lte, uint8_t cid);

/**
 * @brief 检查 PDP 状态
 */
int xy_lte_is_pdp_active(xy_lte_t *lte, uint8_t cid);

/**
 * @brief TCP/UDP 连接
 */
int xy_lte_connect(xy_lte_t *lte, uint8_t link_id, const char *server, uint16_t port, bool tcp);

/**
 * @brief 关闭连接
 */
int xy_lte_close(xy_lte_t *lte, uint8_t link_id);

/**
 * @brief 发送数据
 */
int xy_lte_send(xy_lte_t *lte, uint8_t link_id, const uint8_t *data, size_t len);

/**
 * @brief 接收数据
 */
int xy_lte_recv(xy_lte_t *lte, uint8_t link_id, uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief 注册 URC 回调
 */
int xy_lte_register_urc(xy_lte_t *lte, xy_lte_urc_callback_t callback);

/**
 * @brief 注册接收回调
 */
int xy_lte_register_recv(xy_lte_t *lte, xy_lte_recv_callback_t callback);

/**
 * @brief 发送 AT 命令
 */
int xy_lte_send_at(xy_lte_t *lte, const char *cmd, char *response, size_t resp_len, uint32_t timeout);

/**
 * @brief 重启模块
 */
int xy_lte_reboot(xy_lte_t *lte);

/**
 * @brief 获取 IP 地址
 */
int xy_lte_get_ip(xy_lte_t *lte, char *ip, size_t len);

/**
 * @brief 获取 IMEI
 */
int xy_lte_get_imei(xy_lte_t *lte, char *imei, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_LTE_H */
