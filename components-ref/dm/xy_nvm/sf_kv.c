#include "sf_kv.h"

#define FLASH_PAGE_SIZE 512
#define FLASH_KV_PAGE 3
// #define ALIGNED_4(size, align)    (((size) + (align) - 1) & ~((align) - 1))
#define ALIGNED_4(size)    ((size) + 3) & ~(3)

#define KV_SUM_SIZE (FLASH_PAGE_SIZE * (FLASH_KV_PAGE - 1))
#define KV_BASE_ADDR    0x00
#define KV_BACK_ADDR    (KV_BASE_ADDR + KV_SUM_SIZE)

// PACK除了数据以外的字节 头字节(4byte) + (key_id + is_en + len + sum)(4byte)
#define KV_PACK_NO_DATA_BYTE  8
#define KV_PACK_HEAD_BYTE     4
#define KV_PACK_INFO_BYTE     4 // (key_id + is_en + len + sum)(4byte)


static const char valid_space[9] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};



static sf_uint8_t compute_checksum(kv_t *kv)
{
    sf_uint8_t i;
    sf_uint16_t sum = 0;
    sum = kv->key_id + kv->is_en + kv->len;
    for (i = 0; i < (kv->len-KV_PACK_INFO_BYTE); i++)
        sum += *(kv->buff + i);
    return (sf_uint8_t)(sum & 0xff);
}


static void read_flash_pack(kv_t *kv, sf_uint32_t addr)
{
    memcpy(kv, (kv_t *)(addr), sizeof(kv_t)-4); //拷贝除了head部分？
    kv->buff = (sf_uint8_t *)(addr + KV_PACK_HEAD_BYTE + KV_PACK_INFO_BYTE);
}

/**
 * @brief
 *
 * @param kv_head
 * @param location 当前flash位置
 * @return sf_bool
 */
static sf_bool check_valid(kv_t *kv_head, sf_uint32_t *location)
{
    sf_uint8_t sum;
    if (kv_head->head != KV_SYS_PACK_HEAD) {
        *location += 4;
        return false;
    }

    // 先确保数据完整性，才能进行判断。4-->头字节(4byte)  |  3-->(key_id + is_en + len)
    sum = = compute_checksum(kv_head);

    if (kv_head->sum != sum) {
        // 指针移动，寻找头。头字节(4byte) + (key_id + is_en + len + sum)(4byte)
        // 为什么不直接移动倒下一个位置？因为无法确保len位有效
        *location += KV_PACK_NO_DATA_BYTE; // 8byte
        return false;
    }

    // 0xff是为写入的值
    if (kv_head->is_en != 0xFF) {
        *location += (kv_head->len + KV_PACK_INFO_BYTE);
        return false;
    }

    return true;
}

static void* find_blank_addr(viod)
{
    sf_uint32_t i = 0;
    sf_uint8_t sum;
    while (i < KV_SUM_SIZE) {
        kv_t kv_head;
        read_flash_pack(&kv_head, KV_BACK_ADDR + i);
        if (kv_head == KV_SYS_PACK_HEAD) {
            sum = compute_checksum(&kv_head);

            if (kv_head.sum != sum) {
                i += KV_PACK_NO_DATA_BYTE;
            } else {
                // 指针移动到下一个数据包头 数据长度 + KV_PACK_NO_DATA_BYTE
                i += kv_head.len + KV_PACK_NO_DATA_BYTE;
            }
            goto end;
        } else if (kv_head.head == 0xFFFFFFFF) {
            // KV_PACK_NO_DATA_BYTE + 1 byte
            if ((i + 9) >= KV_SUM_SIZE)
                return NULL;
            if (memcmp((void*)(KV_BASE_ADDR + i), valid_space, 9) == 0)
                return (void*)(KV_BASE_ADDR + i);
        }
        // 指针移动倒下一个字
        i += 4;
end:
        i = ALIGNED_4(i);
    }
    return NULL;
}

static void *find_kv_addr(uint8_t key_id)
{
    sf_uint32_t i = 0;
    while (i < KV_SUM_SIZE)
    {
        kv_t kv_head;
        read_flash_pack(&kv_head, KV_BASE_ADDR + i);

        // 判断 flash 数据是否有效
        if (!check_valid(&kv_head, &i))
            goto end;

        // 判断键值是否匹配
        if (kv_head.key_id != key_id) {
            i += kv_head.len + KV_PACK_NO_DATA_BYTE;
            goto end;
        } else {
            return (void*)(KV_BACK_ADDR+i);
        }
end:
        i = ALIGNED_4(i);
    }
    return NULL;
}

sf_int8_t kv_gc_env(void)
{
    // bsp_flash_earase_page()
    sf_uint8_t kv_page_tick = 0;
    sf_uint32_t back_byte = 0;
    sf_uint32_t i;
    kv_t kv_head;
    while(i < KV_SUM_SIZE)
    {
        read_flash_pack(&kv_head, KV_BACK_ADDR + i);

        if (!check_valid(&kv_head, &i))
            goto end;
    }
}

sf_uint8_t ke_set(sf_uint8_t key_id, void *data, sf_uint8_t len)
{
    static sf_uint8_t kv_set_state = 0;

    kv_t kv_tmp = {0}

    if (key_id == 0 || key_id == 255)
        return 1;

    if ( (kv_set_state) || (KV_SUM_SIZE == 0))
        return 2;

    kv_set_state = 1;
}