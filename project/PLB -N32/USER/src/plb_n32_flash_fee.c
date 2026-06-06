#include "plb_n32_flash_fee.h"

#include "n32l40x_flash.h"
#include "xy_string.h"

typedef struct {
    uint32_t base_addr;
    uint32_t total_size;
    uint32_t page_size;
    uint32_t page_count;
} plb_n32_flash_ctx_t;

#define PLB_N32_EEPROM_ITEM_BOOT_COUNT 0u
#define PLB_N32_EEPROM_ITEM_COUNT      1u
#define PLB_N32_FEE_KEY_SERVER         0x00020001u

static eflash_t s_plb_n32_fee;
static bool s_plb_n32_fee_page_erased[PLB_N32_FEE_PAGE_COUNT];
static xy_eeprom_t s_plb_n32_eeprom;
static uint32_t s_plb_n32_eeprom_shadow[PLB_N32_EEPROM_ITEM_COUNT];
static uint8_t s_plb_n32_eeprom_valid[XY_EEPROM_BITMAP_SIZE(PLB_N32_EEPROM_ITEM_COUNT)];
static uint32_t s_plb_n32_boot_count;

static plb_n32_flash_ctx_t s_plb_n32_fee_ctx = {
    PLB_N32_FEE_BASE_ADDR,
    PLB_N32_FEE_TOTAL_SIZE,
    PLB_N32_FLASH_PAGE_SIZE,
    PLB_N32_FEE_PAGE_COUNT,
};

static plb_n32_flash_ctx_t s_plb_n32_eeprom_ctx = {
    PLB_N32_EEPROM_BASE_ADDR,
    PLB_N32_EEPROM_TOTAL_SIZE,
    PLB_N32_FLASH_PAGE_SIZE,
    PLB_N32_EEPROM_PAGE_COUNT,
};

static int flash_range_ok(const plb_n32_flash_ctx_t *ctx, uint32_t offset,
                          size_t size)
{
    if (ctx == NULL || offset > ctx->total_size) {
        return 0;
    }
    if (size > (size_t)(ctx->total_size - offset)) {
        return 0;
    }
    return 1;
}

static eflash_result_t flash_read_fee(void *ctx, uint32_t offset,
                                      uint8_t *data, size_t size)
{
    plb_n32_flash_ctx_t *flash = (plb_n32_flash_ctx_t *)ctx;
    if (data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }
    if (!flash_range_ok(flash, offset, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }
    memcpy(data, (const void *)(flash->base_addr + offset), size);
    return EFLASH_OK;
}

static eflash_result_t flash_write_fee(void *ctx, uint32_t offset,
                                       const uint8_t *data, size_t size)
{
    plb_n32_flash_ctx_t *flash = (plb_n32_flash_ctx_t *)ctx;
    if (data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }
    if (!flash_range_ok(flash, offset, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }
    if ((offset & 0x3u) != 0u || (size & 0x3u) != 0u) {
        return EFLASH_ERROR_ALIGNMENT;
    }
    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) {
        return EFLASH_ERROR_BUSY;
    }

    FLASH_Unlock();
    for (size_t i = 0; i < size; i += 4u) {
        uint32_t word;
        memcpy(&word, data + i, sizeof(word));
        if (FLASH_ProgramWord(flash->base_addr + offset + i, word)
            != FLASH_COMPL) {
            FLASH_Lock();
            return EFLASH_ERROR_WRITE_FAIL;
        }
    }
    FLASH_Lock();
    return EFLASH_OK;
}

static eflash_result_t flash_erase_page_fee(void *ctx, uint32_t page_index)
{
    plb_n32_flash_ctx_t *flash = (plb_n32_flash_ctx_t *)ctx;
    if (flash == NULL || page_index >= flash->page_count) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }
    if (FLASH_ClockInit() == FLASH_HSICLOCK_DISABLE) {
        return EFLASH_ERROR_BUSY;
    }

    FLASH_Unlock();
    if (FLASH_EraseOnePage(flash->base_addr + page_index * flash->page_size)
        != FLASH_COMPL) {
        FLASH_Lock();
        return EFLASH_ERROR_ERASE_FAIL;
    }
    FLASH_Lock();
    return EFLASH_OK;
}

static xy_eeprom_result_t flash_read_eeprom(void *ctx, uint32_t offset,
                                            uint8_t *data, size_t size)
{
    eflash_result_t ret = flash_read_fee(ctx, offset, data, size);
    return ret == EFLASH_OK ? XY_EEPROM_OK : XY_EEPROM_ERROR_READ;
}

static xy_eeprom_result_t flash_write_eeprom(void *ctx, uint32_t offset,
                                             const uint8_t *data, size_t size)
{
    eflash_result_t ret = flash_write_fee(ctx, offset, data, size);
    return ret == EFLASH_OK ? XY_EEPROM_OK : XY_EEPROM_ERROR_WRITE;
}

static xy_eeprom_result_t flash_erase_page_eeprom(void *ctx, uint32_t page_index)
{
    eflash_result_t ret = flash_erase_page_fee(ctx, page_index);
    return ret == EFLASH_OK ? XY_EEPROM_OK : XY_EEPROM_ERROR_ERASE;
}

static const eflash_ops_t s_plb_n32_fee_ops = {
    flash_read_fee,
    flash_write_fee,
    flash_erase_page_fee,
};

static const xy_eeprom_ops_t s_plb_n32_eeprom_ops = {
    flash_read_eeprom,
    flash_write_eeprom,
    flash_erase_page_eeprom,
};

eflash_result_t plb_n32_flash_read(uint32_t offset, uint8_t *data, size_t size)
{
    return flash_read_fee(&s_plb_n32_fee_ctx, offset, data, size);
}

eflash_result_t plb_n32_flash_write(uint32_t offset, const uint8_t *data,
                                    size_t size)
{
    return flash_write_fee(&s_plb_n32_fee_ctx, offset, data, size);
}

eflash_result_t plb_n32_flash_erase_page(uint32_t page_index)
{
    return flash_erase_page_fee(&s_plb_n32_fee_ctx, page_index);
}

eflash_result_t plb_n32_fee_init(void)
{
    const eflash_config_t config = {
        PLB_N32_FEE_TOTAL_SIZE,
        PLB_N32_FLASH_PAGE_SIZE,
        PLB_N32_FEE_PAGE_COUNT,
        EFLASH_WRITE_UNIT_32BIT,
        false,
    };

    return eflash_init_with_ops(&s_plb_n32_fee, &config,
                                (uint8_t *)PLB_N32_FEE_BASE_ADDR,
                                s_plb_n32_fee_page_erased,
                                &s_plb_n32_fee_ops,
                                &s_plb_n32_fee_ctx);
}

eflash_t *plb_n32_fee(void)
{
    return &s_plb_n32_fee;
}

xy_eeprom_result_t plb_n32_eeprom_init(void)
{
    const xy_eeprom_config_t config = {
        PLB_N32_FLASH_PAGE_SIZE,
        PLB_N32_EEPROM_PAGE_COUNT,
        PLB_N32_EEPROM_ITEM_COUNT,
        sizeof(s_plb_n32_eeprom_shadow[0]),
        4u,
    };

    xy_eeprom_result_t ret = xy_eeprom_init(&s_plb_n32_eeprom, &config,
                                            &s_plb_n32_eeprom_ops,
                                            &s_plb_n32_eeprom_ctx,
                                            (uint8_t *)s_plb_n32_eeprom_shadow,
                                            s_plb_n32_eeprom_valid,
                                            sizeof(s_plb_n32_eeprom_valid));
    if (ret == XY_EEPROM_ERROR_CORRUPT) {
        ret = xy_eeprom_reset(&s_plb_n32_eeprom);
        if (ret == XY_EEPROM_OK) {
            ret = xy_eeprom_init(&s_plb_n32_eeprom, &config,
                                 &s_plb_n32_eeprom_ops,
                                 &s_plb_n32_eeprom_ctx,
                                 (uint8_t *)s_plb_n32_eeprom_shadow,
                                 s_plb_n32_eeprom_valid,
                                 sizeof(s_plb_n32_eeprom_valid));
        }
    }
    return ret;
}

xy_eeprom_t *plb_n32_eeprom(void)
{
    return &s_plb_n32_eeprom;
}

eflash_result_t plb_n32_boot_count_update(uint32_t *boot_count)
{
    uint32_t last_count = 0u;
    if (xy_eeprom_read(&s_plb_n32_eeprom, PLB_N32_EEPROM_ITEM_BOOT_COUNT,
                       &last_count) != XY_EEPROM_OK) {
        last_count = 0u;
    }

    s_plb_n32_boot_count = last_count + 1u;
    if (xy_eeprom_write(&s_plb_n32_eeprom, PLB_N32_EEPROM_ITEM_BOOT_COUNT,
                        &s_plb_n32_boot_count) != XY_EEPROM_OK) {
        return EFLASH_ERROR_WRITE_FAIL;
    }

    if (boot_count != NULL) {
        *boot_count = s_plb_n32_boot_count;
    }
    return EFLASH_OK;
}

uint32_t plb_n32_boot_count_get(void)
{
    return s_plb_n32_boot_count;
}

eflash_result_t plb_n32_server_endpoint_load(plb_n32_server_endpoint_t *endpoint)
{
    if (endpoint == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    eflash_result_t ret = eflash_read(&s_plb_n32_fee, PLB_N32_FEE_KEY_SERVER,
                                      (uint8_t *)endpoint, sizeof(*endpoint));
    if (ret == EFLASH_OK) {
        return EFLASH_OK;
    }

    const plb_n32_server_endpoint_t def = {
        {192u, 168u, 1u, 100u},
        8883u,
        0u,
    };
    *endpoint = def;
    ret = eflash_write(&s_plb_n32_fee, PLB_N32_FEE_KEY_SERVER,
                       (const uint8_t *)endpoint, sizeof(*endpoint));
    if (ret != EFLASH_OK) {
        ret = eflash_erase_all(&s_plb_n32_fee);
        if (ret != EFLASH_OK) {
            return ret;
        }
        ret = eflash_write(&s_plb_n32_fee, PLB_N32_FEE_KEY_SERVER,
                           (const uint8_t *)endpoint, sizeof(*endpoint));
    }
    return ret;
}

eflash_result_t plb_n32_server_endpoint_save(const plb_n32_server_endpoint_t *endpoint)
{
    if (endpoint == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }
    return eflash_write(&s_plb_n32_fee, PLB_N32_FEE_KEY_SERVER,
                        (const uint8_t *)endpoint, sizeof(*endpoint));
}
