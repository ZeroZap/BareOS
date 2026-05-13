#ifndef _SF_KV_H_
#define _SF_KV_H_

#include "sf.h"
/* Define --------------------------------------------------------------------*/
#define FLASH_ONE_PAGE_BYTE 32
#define KV_SYS_PACK_HEAD    0xFEEF9581
/* Public Struct -------------------------------------------------------------*/
// 仅适用简易版
typedef struct {
    sf_uint8_t  len;                                    // 实际长度(包含key_id+is_en+buff)
    sf_uint8_t  key_id;                                 // KEY ID [1,254]  0和255不能使用
    sf_uint8_t  is_en;                                  // 是否有效 0--无效 FF--有效
    sf_uint8_t  buff[FLASH_ONE_PAGE_BYTE - 4];    // 实际数据（最大长度）
    sf_uint8_t  sum;                                    // 校验和
} kv_sys_t;

// 仅适用终极版 FE EF 95 81 KEYID is_en len data0 data1 sum
typedef struct {
    sf_uint32_t head;                                   // 头字节 0xFEEF9581(固有)
    sf_uint8_t  sum;                                    // 校验和
    sf_uint8_t  len;                                    // 实际长度(包含key_id+is_en+buff)
    sf_uint8_t  key_id;                                 // KEY ID [1,254]  0和255不能使用
    sf_uint8_t  is_en;                                  // 是否有效 0--无效 FF--有效
    sf_uint8_t  *buff;                                  // 实际数据指针
} kv_t;

sf_uint8_t sf_kv_gc_env(void); //垃圾回收
void sf_kv_gc_check(void);
void *sf_kv_get(sf_uint8_t key_id);
void *sf_kv_set(sf_uint8_t key_id, void *data, sf_uint8_t len);
void *sf_kv_del(sf_uint8_t key_id);

#endif