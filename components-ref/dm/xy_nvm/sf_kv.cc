/********************************************************************************
 * @file    kv_sys_m.c
 * @author  jianqiang.xue
 * @version V1.0.0
 * @date    2022-09-14
 * @brief   KV键值最小系统(不固定长度) https://lisun.blog.csdn.net/article/details/121140849
 * 1. 读取时，一定是4字节对齐的。即读取0xff00，正确。 读取0xff01，错误。会导致程序跑飞。
 * 2. todo 未完成的东西，数据指针转换为实体返回。
 ********************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bsp_flash.h"

/* Private Includes ----------------------------------------------------------*/
#include "kv_sys.h"

#ifdef LISUN_SDK
#include "ls_syscfg.h"
#else
#define LS_FLASH_PAGE_SIZE    512
#define LS_FLASH_KV_PAGE      3
#endif

/* Private Define ------------------------------------------------------------*/
// KV系统总共可以使用N字节
#define LS_KV_SUM_SIZE        (LS_FLASH_PAGE_SIZE * (LS_FLASH_KV_PAGE - 1))

#ifndef LISUN_SDK
#define LS_KV_BASE_ADDR       0x00 // 自行修改
#define LS_KV_BACK_ADDR       (LS_KV_BASE_ADDR + LS_KV_SUM_SIZE)
#endif

// PACK除了数据以外的字节 头字节(4byte) + (key_id + is_en + len + sum)(4byte)
#define KV_PACK_NO_DATA_BYTE  8
#define KV_PACK_HEAD_BYTE     4
#define KV_PACK_INFO_BYTE     4 // (key_id + is_en + len + sum)(4byte)

// 对齐4字节,不足4字节，补齐4字节
#define ALIGNED_4(num)       if ((num % 4) > 0) num += (4 - (i % 4));

static const char valid_space[9] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/* Private Function Prototypes -----------------------------------------------*/
/**
 * @brief  计算校验值（累计两段值）
 * @param  data: 待校验的数据头指针
 * @param  len: 数据长度
 * @retval 校验和(低八位)
 */
static uint8_t compute_checksum(kv_sys_m_t* kv_head) {
    uint16_t sum = 0;
    sum = kv_head->key_id + kv_head->is_en + kv_head->len;
    for (uint8_t i = 0; i < kv_head->len - KV_PACK_INFO_BYTE; i++)
        sum += *(kv_head->buff + i);
    return (uint8_t)(sum & 0x00FF);
}

static void read_flash_pack(kv_sys_m_t* kv, uint32_t addr) {
    memcpy(kv, (kv_sys_m_t*)(addr), sizeof(kv_sys_m_t) - 4);
    kv->buff = (uint8_t*)(addr + KV_PACK_HEAD_BYTE + KV_PACK_INFO_BYTE);
}

/**
 * @brief  验证区域有效性
 * @param  kv_head: 数据头指针
 * @param  location: 当前flash位置(相对位置)
 * @retval 0--失败  1--成功
 */
static bool check_valid(kv_sys_m_t* kv_head, uint32_t *location) {
    if (kv_head->head != KV_SYS_PACK_HEAD) {
        *location += 4;
        return false;
    }
    // 先确保数据完整性，才能进行判断。4-->头字节(4byte)  |  3-->(key_id + is_en + len)
    uint8_t sum = compute_checksum(kv_head);
    if (kv_head->sum != sum) {
        // 指针移动，寻找头。 头字节(4byte) + (key_id + is_en + len + sum)(4byte)
        // (为什么不直接移动到下个位置？因为无法确保len位有效)
        *location += KV_PACK_NO_DATA_BYTE;
        return false;
    }
    if (kv_head->is_en != 0xFF) {
        // 指针移动到下一个数据包头 数据长度
        *location += (kv_head->len + KV_PACK_NO_DATA_BYTE);
        return false;
    }
    return true;
}

// 寻找指定键值
static void* find_kv_addr(uint8_t key_id) {
    uint32_t i = 0;
    while (i < LS_KV_SUM_SIZE) {
        kv_sys_m_t kv_head;
        read_flash_pack(&kv_head, LS_KV_BASE_ADDR + i);
        // 判断flash数据是否有效
        if (!check_valid(&kv_head, &i))
            goto end;
        // 判断键值是否匹配
        if (kv_head.key_id != key_id) {
            // 指针移动到下一个数据包头 数据长度 + KV_PACK_NO_DATA_BYTE
            i += (kv_head.len + KV_PACK_NO_DATA_BYTE);
            goto end;
        } else
            return (void*)(LS_KV_BASE_ADDR + i);
        // 指针移动到下一个字
        i += 4;
end:
        ALIGNED_4(i);
    }
    return NULL;
}

// 寻找空白区
static void* find_blank_addr(void) {
    uint32_t i = 0;
    while (i < LS_KV_SUM_SIZE) {
        kv_sys_m_t kv_head;
        read_flash_pack(&kv_head, LS_KV_BASE_ADDR + i);
        if (kv_head.head == KV_SYS_PACK_HEAD) {
            // 先确保数据完整性，才能进行判断。
            uint8_t sum = compute_checksum(&kv_head);
            if (kv_head.sum != sum) {
                // 指针移动，寻找头。 (为什么不直接移动到下个位置？因为无法确保len位有效)
                i += KV_PACK_NO_DATA_BYTE;
            } else {
                // 指针移动到下一个数据包头 数据长度 + KV_PACK_NO_DATA_BYTE
                i += (kv_head.len + KV_PACK_NO_DATA_BYTE);
            }
            goto end;
        } else if (kv_head.head == 0xFFFFFFFF) {
            // 确保至少有9个有效字节，能存放一个字节
            if ((i + 9) >= LS_KV_SUM_SIZE)
                return NULL;
            if (memcmp((void*)(LS_KV_BASE_ADDR + i), valid_space, 9) == 0)
                return (void*)(LS_KV_BASE_ADDR + i);
        }

        // 指针移动到下一个字
        i += 4;
end:
        ALIGNED_4(i);
    }
    return NULL;
}

/* Public Function Prototypes ------------------------------------------------*/

/**
 * @brief  FLASH垃圾回收 (本回收机制，可能导致每页中会出现空白字段，可能写入字段小于空白页，可插入)
 */
int kv_gc_env(void) {
    bsp_flash_erase_page(LS_KV_BACK_ADDR, 1);
    while (bsp_flash_is_busy());

    uint8_t kv_page_tick = 0;
    uint32_t back_byte = 0;

    uint32_t i = 0;
    while (i < LS_KV_SUM_SIZE) {
        kv_sys_m_t kv_head;
        read_flash_pack(&kv_head, LS_KV_BASE_ADDR + i);
        // 判断flash数据是否有效
        if (!check_valid(&kv_head, &i))
            goto end;
        uint8_t wish_write_byte = kv_head.len + KV_PACK_NO_DATA_BYTE;
        // 当备份页满时，清理前面页数,然后备份当前数据
        if (back_byte + wish_write_byte > LS_FLASH_PAGE_SIZE) {
            kv_sys_m_t kv_sys;
            memcpy(&kv_sys, &kv_head, sizeof(kv_sys_m_t));
            kv_sys.buff = malloc(kv_head.len);
            if (kv_sys.buff == NULL) return -2;
            memcpy(kv_sys.buff, kv_head.buff, kv_head.len);
            kv_sys.sum = *(kv_sys.buff + kv_head.len);

            if (kv_page_tick >= LS_FLASH_KV_PAGE - 1) {
                free(kv_sys.buff);
                return -1;
            }
            // 备份页搬运到有效页
            bsp_flash_carry(LS_KV_BASE_ADDR + kv_page_tick * LS_FLASH_PAGE_SIZE, LS_KV_BACK_ADDR, LS_FLASH_PAGE_SIZE);
            kv_page_tick++;
            back_byte = 0;
            // 刚刚备份的有效数据写到到备份区 (由于RAM数据不连续，需要分段写 todo) (4 --> *buff 指针4字节)
            bsp_flash_write_nbyte_s(LS_KV_BACK_ADDR + back_byte, (uint8_t*)&kv_sys, sizeof(kv_sys_m_t) - 4);
            back_byte += sizeof(kv_sys_m_t) - 4;
            bsp_flash_write_nbyte_s(LS_KV_BACK_ADDR + back_byte, kv_sys.buff, kv_sys.len);
            back_byte += kv_sys.len;
            free(kv_sys.buff);
        } else {
            // 搬运有效数据
            bsp_flash_write_nbyte_s(LS_KV_BACK_ADDR + back_byte, (uint8_t*)&kv_head, kv_head.len + KV_PACK_NO_DATA_BYTE);
            back_byte += kv_head.len + KV_PACK_NO_DATA_BYTE;
        }
        // 指针移动到下一个字
        i += 4;
end:
        ALIGNED_4(i);
    }

    if (back_byte != 0) {
        if (kv_page_tick >= LS_FLASH_KV_PAGE - 1)
            return -1;
        bsp_flash_carry(LS_KV_BASE_ADDR + kv_page_tick * LS_FLASH_PAGE_SIZE, LS_KV_BACK_ADDR, LS_FLASH_PAGE_SIZE);
        kv_page_tick++;
        back_byte = 0;
    }
    // 清理未使用的空间
    for (uint8_t i = kv_page_tick; i < LS_FLASH_KV_PAGE - 1; i++)
        bsp_flash_erase_page(LS_KV_BASE_ADDR + kv_page_tick * LS_FLASH_PAGE_SIZE, 1);
    while (bsp_flash_is_busy());
    return 0;
}

/**
 * @brief  [上电调用] 检测当前KV键值是否异常 如：掉电导致异常，则进行数据恢复
 */
void kv_gc_check(void) {
    kv_sys_m_t* kv = find_blank_addr(); // 得到空白块
    // 如果数据满了，则进行垃圾回收处理
    if (kv == NULL)
        kv_gc_env();
    else {
        // 判断备份区是否有残余
        kv = (kv_sys_m_t*)LS_KV_BACK_ADDR;
        if (kv->head == KV_SYS_PACK_HEAD) {
            for (uint32_t i = 0; i < LS_FLASH_PAGE_SIZE; i++) {
                kv_sys_m_t kv_head;
                read_flash_pack(&kv_head, LS_KV_BACK_ADDR + i);
                if (!check_valid(&kv_head, &i))
                    continue;
                kv_set_env(kv_head.key_id, kv_head.buff, kv_head.len);
            }
            bsp_flash_erase_page(LS_KV_BACK_ADDR, 1);
        }
    }
}

/**
 * @brief  从FLASH中获取KV值
 * @param  key_id: KEY ID
 * @retval 数据指针
 */
void* kv_get_env(uint8_t key_id) {
    if (key_id == 0 || key_id == 255)
        return NULL;
    kv_sys_m_t *kv = (kv_sys_m_t*)find_kv_addr(key_id);
    return kv != NULL ? &(kv->buff) : NULL;
}

/**
 * @brief  从FLASH中删除某KV值
 * @param  key_id: KEY ID
 * @retval 1--成功 0--失败
 */
bool kv_del_env(uint8_t key_id) {
    bool state = false;
    while (1) {
        kv_sys_m_t* kv_head = (kv_sys_m_t*)find_kv_addr(key_id);
        if (kv_head == NULL) break;
        uint32_t temp_addr = (uint32_t)kv_head + 7;
        state = bsp_flash_write_byte(temp_addr, 0x00); // 将之前值标记为无效
    }
    return state;
}

/**
 * @brief  KV值写入Flash
 * @param  key_id: KEY ID
 * @param  *data: 数组指针
 * @param  len: 数据长度
 */
bool kv_set_env(uint8_t key_id, void* data, uint8_t len) {
    static bool kv_set_state = false;  // false--free   true--bus
    kv_sys_m_t kv_sys_temp = {0};
    // 检测ID是否异常
    if (key_id == 0 || key_id == 255)
        return false;
    // 检测参数和当前状态是否异常
    if ((kv_set_state) || (LS_KV_SUM_SIZE == 0))
        return false;
    kv_set_state = true;
    // 检测KEY_ID是否存在
    // 判断数据是否相同
    uint8_t* old = kv_get_env(key_id);
    if (old != NULL) {
        if (memcmp(data, old, len) == 0) {
            // 数据一致，直接返回
            kv_set_state = false;
            return true;
        }
    }
    kv_del_env(key_id);
    // 得到空白块
    kv_sys_m_t* kv = find_blank_addr();
    // 如果数据满了，则进行垃圾回收处理
    if (kv == NULL) {
        kv_gc_env();
        kv = find_blank_addr();
    }
    // 填充数据
    kv_sys_temp.head   = KV_SYS_PACK_HEAD;
    kv_sys_temp.key_id = key_id;
    kv_sys_temp.len    = len + KV_PACK_INFO_BYTE;
    kv_sys_temp.is_en  = 0xFF;
    kv_sys_temp.buff   = data;
    kv_sys_temp.sum    = compute_checksum(&kv_sys_temp);
    bsp_flash_write_nbyte_s((uint32_t)kv, (uint8_t*)&kv_sys_temp, sizeof(kv_sys_m_t) - 4);
    bsp_flash_write_nbyte_s((uint32_t)kv + (sizeof(kv_sys_m_t) - 4), data, len);
    kv_set_state = false;
    return true;
}

