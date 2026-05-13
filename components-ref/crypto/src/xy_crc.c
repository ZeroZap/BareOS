#include "xy_crc.h"
#include "xy_crc_cfg.h"

// Core bit manipulation functions
static uint8_t reflect8(uint8_t value)
{
    uint8_t reflected = 0;
    for (int i = 0; i < 8; ++i) {
        if (value & (1 << i))
            reflected |= (1 << (7 - i));
    }
    return reflected;
}

static uint64_t reflect(uint64_t value, uint8_t width)
{
    uint64_t reflected = 0;
    for (uint8_t i = 0; i < width; i++) {
        if (value & (1ULL << i))
            reflected |= 1ULL << (width - 1 - i);
    }
    return reflected;
}

// Core CRC calculation functions
uint64_t xy_crc_calc(const xy_crc_cfg_t *cfg, const uint8_t *data,
                     uint16_t length)
{
    if (!cfg || !data || cfg->width > 64 || cfg->width < 2)
        return 0;

    uint64_t crc = cfg->init_value;
    uint64_t mask =
        (cfg->width < 64) ? ((1ULL << cfg->width) - 1) : 0xFFFFFFFFFFFFFFFFULL;

    while (length--) {
        uint8_t byte = *data++;
        if (cfg->ref_in)
            byte = reflect8(byte);

        crc ^= ((uint64_t)byte << (cfg->width - 8));
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & (1ULL << (cfg->width - 1))) {
                crc = (crc << 1) ^ cfg->polynomial;
            } else {
                crc <<= 1;
            }
            crc &= mask;
        }
    }

    if (cfg->ref_out)
        crc = reflect(crc, cfg->width);

    return (crc ^ cfg->xor_out) & mask;
}

uint64_t xy_crc_calc_table(const xy_crc_cfg_t *cfg, const uint64_t *table,
                           const uint8_t *data, uint16_t length)
{
    if (!cfg || !table || !data || cfg->width > 64 || cfg->width < 2)
        return 0;

    uint64_t crc = cfg->init_value;
    uint64_t mask =
        (cfg->width < 64) ? ((1ULL << cfg->width) - 1) : 0xFFFFFFFFFFFFFFFFULL;

    while (length--) {
        uint8_t byte = *data++;
        if (cfg->ref_in)
            byte = reflect8(byte);
        crc = table[(uint8_t)(crc ^ byte)] & mask;
    }

    if (cfg->ref_out)
        crc = reflect(crc, cfg->width);

    return (crc ^ cfg->xor_out) & mask;
}

int xy_crc_make_table(const xy_crc_cfg_t *cfg, uint64_t *table)
{
    if (!cfg || !table || cfg->width > 64 || cfg->width < 2)
        return -1;

    uint64_t mask =
        (cfg->width < 64) ? ((1ULL << cfg->width) - 1) : 0xFFFFFFFFFFFFFFFFULL;

    for (int i = 0; i < 256; i++) {
        uint64_t crc = (uint64_t)i << (cfg->width - 8);
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & (1ULL << (cfg->width - 1))) {
                crc = (crc << 1) ^ cfg->polynomial;
            } else {
                crc <<= 1;
            }
            crc &= mask;
        }
        table[i] = crc;
    }

    return 0;
}

// Hardware support section
#if defined(XY_CRC_HW_SUPPORT)
static uint64_t xy_crc_calc_hw(const xy_crc_cfg_t *cfg, const uint8_t *data,
                               uint16_t length, uint8_t use_dma)
{
    // Hardware specific implementation
    if (use_dma) {
        // DMA based calculation
    } else {
        // Direct hardware calculation
    }
    return 0;
}
#endif

// Extended calculation interface
uint64_t xy_crc_calc_ex(const xy_crc_cfg_t *cfg, const uint8_t *data,
                        uint16_t length, const xy_crc_opt_t *opt)
{
    static uint64_t crc_table[256];
    static uint8_t table_initialized = 0;

    if (!cfg || !data || !opt || cfg->width > 64 || cfg->width < 2)
        return 0;

    switch (opt->method) {
    case XY_CRC_METHOD_SW:
        return xy_crc_calc(cfg, data, length);

    case XY_CRC_METHOD_TABLE:
        if (!table_initialized) {
            xy_crc_make_table(cfg, crc_table);
            table_initialized = 1;
        }
        return xy_crc_calc_table(cfg, crc_table, data, length);

    case XY_CRC_METHOD_HW:
#if defined(XY_CRC_HW_SUPPORT)
        return xy_crc_calc_hw(cfg, data, length, opt->use_dma);
#else
        return xy_crc_calc(cfg, data, length); // Fallback to SW
#endif

    default:
        return xy_crc_calc(cfg, data, length); // Default to SW
    }
}

#if XY_CRC_VERIFY_ENABLE
// CRC result verification
static int xy_crc_verify_result(const xy_crc_cfg_t *cfg, uint64_t result,
                                const uint8_t *data, uint16_t length)
{
    // Verify CRC result by recalculating with different method
    uint64_t verify = xy_crc_calc(cfg, data, length);
    return (result == verify) ? 0 : -1;
}
#endif

// CRC variant implementations (grouped by width)
// CRC2 implementations
#if XY_CRC2_SUPPORT

#if XY_CRC2_G704_ENABLE
uint8_t xy_crc2_g704(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 2,
                                      .polynomial = 0x03,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC2_GSM_ENABLE
uint8_t xy_crc2_gsm(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 2,
                                      .polynomial = 0x03,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#endif // XY_CRC2_SUPPORT

// CRC3 implementations
#if XY_CRC3_SUPPORT

#if XY_CRC3_ROHC_ENABLE
uint8_t xy_crc3_rohc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 3,
                                      .polynomial = 0x03,
                                      .init_value = 0x07,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC3_GSM_ENABLE
uint8_t xy_crc3_gsm(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 3,
                                      .polynomial = 0x03,
                                      .init_value = 0x00,
                                      .xor_out    = 0x07,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#endif // XY_CRC3_SUPPORT

// Add conditional compilation for each CRC width group
#if XY_CRC8_SUPPORT

#if XY_CRC8_DVB_S2_ENABLE
uint8_t xy_crc8_dvb_s2(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0xD5,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC8_EBU_ENABLE
uint8_t xy_crc8_ebu(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x1D,
                                      .init_value = 0xFF,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC8_BLUETOOTH_HID_ENABLE
uint8_t xy_crc8_bluetooth_hid(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0xA7,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC8_MIFARE_MAD_ENABLE
uint8_t xy_crc8_mifare_mad(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x1D,
                                      .init_value = 0xC7,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}
#endif

#endif // XY_CRC8_SUPPORT

// CRC4 implementations
uint8_t xy_crc4_itu(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 4,
                                      .polynomial = 0x03,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc4_interlaken(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 4,
                                      .polynomial = 0x03,
                                      .init_value = 0x0F,
                                      .xor_out    = 0x0F,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

// CRC5 implementations
uint8_t xy_crc5_epc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 5,
                                      .polynomial = 0x09,
                                      .init_value = 0x09,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc5_itu(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 5,
                                      .polynomial = 0x15,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc5_usb(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 5,
                                      .polynomial = 0x05,
                                      .init_value = 0x1F,
                                      .xor_out    = 0x1F,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

// CRC6 implementations
uint8_t xy_crc6_gsm(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 6,
                                      .polynomial = 0x2F,
                                      .init_value = 0x00,
                                      .xor_out    = 0x3F,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc6_cdma2000a(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 6,
                                      .polynomial = 0x27,
                                      .init_value = 0x3F,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc6_g704(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 6,
                                      .polynomial = 0x03,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc6_darc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 6,
                                      .polynomial = 0x19,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

// CRC7 implementations
uint8_t xy_crc7_mmc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 7,
                                      .polynomial = 0x09,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc7_umts(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 7,
                                      .polynomial = 0x45,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc7_darc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 7,
                                      .polynomial = 0x09,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

// CRC8 implementations
uint8_t xy_crc8_1wire(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x31,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_gsm_a(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x1D,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_i_code(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x1D,
                                      .init_value = 0xFD,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_opensafety(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x2F,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_wcdma(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x9B,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_cdma2000(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x9B,
                                      .init_value = 0xFF,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_normal(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x07,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_itu(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x07,
                                      .init_value = 0x00,
                                      .xor_out    = 0x55,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_rohc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x07,
                                      .init_value = 0xFF,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_maxim(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x31,
                                      .init_value = 0x00,
                                      .xor_out    = 0x00,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_sae_j1850(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x1D,
                                      .init_value = 0xFF,
                                      .xor_out    = 0xFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

uint8_t xy_crc8_autosar(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 8,
                                      .polynomial = 0x2F,
                                      .init_value = 0xFF,
                                      .xor_out    = 0xFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint8_t)xy_crc_calc(&cfg, data, length);
}

// CRC16 implementations
uint16_t xy_crc16_ccitt_false(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x1021,
                                      .init_value = 0xFFFF,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_x25(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x1021,
                                      .init_value = 0xFFFF,
                                      .xor_out    = 0xFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_opensafety_b(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x755B,
                                      .init_value = 0x0000,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_profibus_arc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x1DCF,
                                      .init_value = 0xFFFF,
                                      .xor_out    = 0xFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_ibm(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x8005,
                                      .init_value = 0x0000,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_maxim(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x8005,
                                      .init_value = 0x0000,
                                      .xor_out    = 0xFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_modbus(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x8005,
                                      .init_value = 0xFFFF,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_ccitt(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x1021,
                                      .init_value = 0x0000,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_xmodem(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x1021,
                                      .init_value = 0x0000,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_dnp(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x3D65,
                                      .init_value = 0x0000,
                                      .xor_out    = 0xFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

uint16_t xy_crc16_usb(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x8005,
                                      .init_value = 0xFFFF,
                                      .xor_out    = 0xFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}

#if XY_CRC16_ARC_ENABLE
uint16_t xy_crc16_arc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0x8005,
                                      .init_value = 0x0000,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC16_TELEDISK_ENABLE
uint16_t xy_crc16_teledisk(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 16,
                                      .polynomial = 0xA097,
                                      .init_value = 0x0000,
                                      .xor_out    = 0x0000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint16_t)xy_crc_calc(&cfg, data, length);
}
#endif

// CRC32 implementations
uint32_t xy_crc32_normal(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_jamcrc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0x00000000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_c(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x1EDC6F41,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_cksum(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0x00000000,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_iso_hdlc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_d(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0xA833982B,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_posix(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0x00000000,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

uint32_t xy_crc32_mpeg2(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0x00000000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}

#if XY_CRC32_SUPPORT

#if XY_CRC32_AIXM_ENABLE
uint32_t xy_crc32_aixm(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x814141AB,
                                      .init_value = 0x00000000,
                                      .xor_out    = 0x00000000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC32_AUTOSAR_ENABLE
uint32_t xy_crc32_autosar(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0xF4ACFB13,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC32_BASE91D_ENABLE
uint32_t xy_crc32_base91d(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0xA833982B,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC32_BZIP2_ENABLE
uint32_t xy_crc32_bzip2(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x04C11DB7,
                                      .init_value = 0xFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC32_CD_ROM_EDC_ENABLE
uint32_t xy_crc32_cd_rom_edc(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 32,
                                      .polynomial = 0x8001801B,
                                      .init_value = 0x00000000,
                                      .xor_out    = 0x00000000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return (uint32_t)xy_crc_calc(&cfg, data, length);
}
#endif

#endif // XY_CRC32_SUPPORT

// CRC64 implementations
uint64_t xy_crc64_ecma(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x42F0E1EBA9EA3693,
                                      .init_value = 0x0000000000000000,
                                      .xor_out    = 0x0000000000000000,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return xy_crc_calc(&cfg, data, length);
}

uint64_t xy_crc64_jones(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0xAD93D23594C935A9,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0x0000000000000000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return xy_crc_calc(&cfg, data, length);
}

uint64_t xy_crc64_go_ecma(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x42F0E1EBA9EA3693,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFFFFFFFFFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return xy_crc_calc(&cfg, data, length);
}

uint64_t xy_crc64_xz(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x42F0E1EBA9EA3693,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFFFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return xy_crc_calc(&cfg, data, length);
}

uint64_t xy_crc64_iso(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x000000000000001B,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFFFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return xy_crc_calc(&cfg, data, length);
}

#if XY_CRC64_SUPPORT

#if XY_CRC64_MS_ENABLE
uint64_t xy_crc64_ms(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x259C84CBA6426349,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0x0000000000000000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC64_GO_ISO_ENABLE
uint64_t xy_crc64_go_iso(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x000000000000001B,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFFFFFFFFFF,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC64_REDIS_CACHED_ENABLE
uint64_t xy_crc64_redis_cached(uint8_t *data, uint16_t length)
{
    static uint64_t table[256];
    static uint8_t table_initialized = 0;
    static const xy_crc_cfg_t cfg    = { .width      = 64,
                                         .polynomial = 0xAD93D23594C935A9,
                                         .init_value = 0x0000000000000000,
                                         .xor_out    = 0x0000000000000000,
                                         .ref_in     = 1,
                                         .ref_out    = 1 };

    if (!table_initialized) {
        xy_crc_make_table(&cfg, table);
        table_initialized = 1;
    }
    return xy_crc_calc_table(&cfg, table, data, length);
}
#endif

#endif // XY_CRC64_SUPPORT

#if XY_CRC64_WE_ENABLE
uint64_t xy_crc64_we(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0x42F0E1EBA9EA3693,
                                      .init_value = 0xFFFFFFFFFFFFFFFF,
                                      .xor_out    = 0xFFFFFFFFFFFFFFFF,
                                      .ref_in     = 0,
                                      .ref_out    = 0 };
    return xy_crc_calc(&cfg, data, length);
}
#endif

#if XY_CRC64_REDIS_ENABLE
uint64_t xy_crc64_redis(uint8_t *data, uint16_t length)
{
    static const xy_crc_cfg_t cfg = { .width      = 64,
                                      .polynomial = 0xAD93D23594C935A9,
                                      .init_value = 0x0000000000000000,
                                      .xor_out    = 0x0000000000000000,
                                      .ref_in     = 1,
                                      .ref_out    = 1 };
    return xy_crc_calc(&cfg, data, length);
}
#endif
