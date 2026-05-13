/********************************************************************************
* @file    kv_sys.h
* @author  jianqiang.xue
* @version V1.0.0
* @date    2021-11-03
* @brief   KV键值系统
********************************************************************************/

#ifndef __KV_SYS_H__
#define __KV_SYS_H__

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#ifdef LISUN_SDK
#include "ls_syscfg.h"
#else
#define LS_FLASH_KV_ONE_PAGE_BYTE 32
#endif
/* Define --------------------------------------------------------------------*/
#define KV_SYS_PACK_HEAD    0xFEEF9581
/* Public Struct -------------------------------------------------------------*/
// 仅适用简易版
typedef struct {
    uint8_t  len;                                    // 实际长度(包含key_id+is_en+buff)
    uint8_t  key_id;                                 // KEY ID [1,254]  0和255不能使用
    uint8_t  is_en;                                  // 是否有效 0--无效 FF--有效
    uint8_t  buff[LS_FLASH_KV_ONE_PAGE_BYTE - 4];    // 实际数据（最大长度）
    uint8_t  sum;                                    // 校验和
} kv_sys_t;

// 仅适用终极版 FE EF 95 81 KEYID is_en len data0 data1 sum
typedef struct {
    uint32_t head;                                   // 头字节 0xFEEF9581(固有)
    uint8_t  sum;                                    // 校验和
    uint8_t  len;                                    // 实际长度(包含key_id+is_en+buff)
    uint8_t  key_id;                                 // KEY ID [1,254]  0和255不能使用
    uint8_t  is_en;                                  // 是否有效 0--无效 FF--有效
    uint8_t  *buff;                                  // 实际数据指针
} kv_sys_m_t;

/* Public Function Prototypes -----------------------------------------------*/

int kv_gc_env(void);
void kv_gc_check(void);
void* kv_get_env(uint8_t key_id);
bool kv_del_env(uint8_t key_id);
bool kv_set_env(uint8_t key_id, void *data, uint8_t len);
#endif

