/**
 * @file xy_crc.h
 * @author Eugene Chan
 * @brief
 * @version 0.1
 * @date 2025-04-19
 *
 * @copyright Copyright (c) ZeroZap 2025
 * This program is free software: you can redistribute it and/or modify
 */

#ifndef __XY_CRC_H__
#define __XY_CRC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Core CRC types and functions
typedef struct {
    uint8_t width;       // CRC width (2-64 bits)
    uint64_t polynomial; // CRC polynomial
    uint64_t init_value; // Initial value
    uint64_t xor_out;    // Final XOR value
    uint8_t ref_in;      // Reflect input bytes
    uint8_t ref_out;     // Reflect output CRC
} xy_crc_cfg_t;

// Basic CRC calculation functions
uint64_t xy_crc_calc(const xy_crc_cfg_t *cfg, const uint8_t *data,
                     uint16_t length);
uint64_t xy_crc_calc_table(const xy_crc_cfg_t *cfg, const uint64_t *table,
                           const uint8_t *data, uint16_t length);
int xy_crc_make_table(const xy_crc_cfg_t *cfg, uint64_t *table);

// Method selection interface
typedef enum {
    XY_CRC_METHOD_SW,    // Pure software calculation
    XY_CRC_METHOD_TABLE, // Table-based calculation
    XY_CRC_METHOD_HW     // Hardware acceleration
} xy_crc_method_t;

typedef struct {
    xy_crc_method_t method; // Calculation method
    uint8_t use_dma;        // Use DMA for HW calculation
} xy_crc_opt_t;

uint64_t xy_crc_calc_ex(const xy_crc_cfg_t *cfg, const uint8_t *data,
                        uint16_t length, const xy_crc_opt_t *opt);

// CRC variants (grouped by width)

// CRC2 variants
uint8_t xy_crc2_g704(uint8_t *data, uint16_t length); // ITU G.704
uint8_t xy_crc2_gsm(uint8_t *data, uint16_t length);  // GSM signaling

// CRC3 variants
uint8_t xy_crc3_rohc(uint8_t *data, uint16_t length); // ROHC header compression
uint8_t xy_crc3_gsm(uint8_t *data, uint16_t length);  // GSM networks

// CRC4 variants
uint8_t xy_crc4_itu(uint8_t *data, uint16_t length); // ITU G.704
uint8_t xy_crc4_interlaken(uint8_t *data,
                           uint16_t length); // Interlaken protocol

// CRC5 variants
uint8_t xy_crc5_epc(uint8_t *data, uint16_t length);       // EPC Gen2
uint8_t xy_crc5_itu(uint8_t *data, uint16_t length);       // ITU G.704
uint8_t xy_crc5_usb(uint8_t *data, uint16_t length);       // USB protocol
uint8_t xy_crc5_bluetooth(uint8_t *data, uint16_t length); // Bluetooth
uint8_t xy_crc5_darc(uint8_t *data, uint16_t length);      // Data Radio Channel
uint8_t xy_crc5_gsm(uint8_t *data, uint16_t length);       // GSM protocol

// CRC6 variants
uint8_t xy_crc6_itu(uint8_t *data, uint16_t length);       // ITU G.704
uint8_t xy_crc6_gsm(uint8_t *data, uint16_t length);       // GSM protocol
uint8_t xy_crc6_cdma2000a(uint8_t *data, uint16_t length); // CDMA2000-A
uint8_t xy_crc6_cdma2000b(uint8_t *data, uint16_t length); // CDMA2000-B
uint8_t xy_crc6_darc(uint8_t *data, uint16_t length);      // Data Radio Channel
uint8_t xy_crc6_g704(uint8_t *data, uint16_t length);      // ITU G.704
uint8_t xy_crc6_bluetooth(uint8_t *data, uint16_t length); // Bluetooth
uint8_t xy_crc6_itu_k(uint8_t *data, uint16_t length);     // ITU G.704 K
uint8_t xy_crc6_itu_o(uint8_t *data, uint16_t length);     // ITU G.704 O
uint8_t xy_crc6_itu_q(uint8_t *data, uint16_t length);     // ITU G.704 Q
uint8_t xy_crc6_itu_r(uint8_t *data, uint16_t length);     // ITU G.704 R
uint8_t xy_crc6_itu_s(uint8_t *data, uint16_t length);     // ITU G.704 S

// CRC7 variants
uint8_t xy_crc7_mmc(uint8_t *data, uint16_t length);  // MMC/SD
uint8_t xy_crc7_rohc(uint8_t *data, uint16_t length); // ROHC protocol
uint8_t xy_crc7_umts(uint8_t *data, uint16_t length); // UMTS mobile networks
uint8_t xy_crc7_bluetooth(uint8_t *data, uint16_t length); // Bluetooth
uint8_t xy_crc7_darc(uint8_t *data, uint16_t length);      // Data Radio Channel
uint8_t xy_crc7_gsm_b(uint8_t *data, uint16_t length);     // GSM-B

// CRC8 variants
uint8_t xy_crc8_normal(uint8_t *data, uint16_t length);    // Standard CRC8
uint8_t xy_crc8_maxim(uint8_t *data, uint16_t length);     // For DS18B20
uint8_t xy_crc8_rohc(uint8_t *data, uint16_t length);      // For ROHC protocol
uint8_t xy_crc8_itu(uint8_t *data, uint16_t length);       // ITU I.432.1
uint8_t xy_crc8_1wire(uint8_t *data, uint16_t length);     // 1-Wire bus
uint8_t xy_crc8_sae_j1850(uint8_t *data, uint16_t length); // SAE J1850
uint8_t xy_crc8_wcdma(uint8_t *data, uint16_t length);     // WCDMA
uint8_t xy_crc8_autosar(uint8_t *data, uint16_t length);   // Automotive
uint8_t xy_crc8_bluetooth(uint8_t *data, uint16_t length); // Bluetooth HID
uint8_t xy_crc8_cdma2000(uint8_t *data, uint16_t length);  // CDMA2000
uint8_t xy_crc8_darc(uint8_t *data, uint16_t length);      // Data Radio Channel
uint8_t xy_crc8_dvb_s2(uint8_t *data, uint16_t length);    // DVB-S2 standard
uint8_t xy_crc8_gsm_b(uint8_t *data, uint16_t length);     // GSM-B protocol
uint8_t xy_crc8_nrsc_5(uint8_t *data, uint16_t length);    // NRSC-5
uint8_t xy_crc8_aes(uint8_t *data, uint16_t length);       // AES block cipher
uint8_t xy_crc8_ebu(uint8_t *data, uint16_t length);       // EBU tech.
uint8_t xy_crc8_gsm_a(uint8_t *data, uint16_t length);     // GSM-A
uint8_t xy_crc8_i_code(uint8_t *data, uint16_t length);    // I-CODE
uint8_t xy_crc8_lte(uint8_t *data, uint16_t length);       // 3GPP LTE
uint8_t xy_crc8_opensafety(uint8_t *data, uint16_t length); // OpenSAFETY
uint8_t xy_crc8_mifare_mad(uint8_t *data, uint16_t length); // MIFARE MAD

// CRC16 variants
uint16_t xy_crc16_modbus(uint8_t *data, uint16_t length); // For Modbus
uint16_t xy_crc16_ccitt(uint8_t *data, uint16_t length);  // CCITT variant
uint16_t xy_crc16_xmodem(uint8_t *data, uint16_t length); // For XModem
uint16_t xy_crc16_dnp(uint8_t *data, uint16_t length);    // For DNP protocol
uint16_t xy_crc16_ibm(uint8_t *data, uint16_t length);    // IBM SDLC
uint16_t xy_crc16_maxim(uint8_t *data, uint16_t length);  // Maxim protocol
uint16_t xy_crc16_usb(uint8_t *data, uint16_t length);    // USB protocol
uint16_t xy_crc16_x25(uint8_t *data, uint16_t length);    // X.25 protocol
uint16_t xy_crc16_ccitt_false(uint8_t *data, uint16_t length); // CCITT FALSE
uint16_t xy_crc16_mcrf4xx(uint8_t *data, uint16_t length);     // MCRF4XX
uint16_t xy_crc16_profibus(uint8_t *data, uint16_t length);    // PROFIBUS
uint16_t xy_crc16_arc(uint8_t *data, uint16_t length);         // ARC/LHA/ANSI
uint16_t xy_crc16_aug_ccitt(uint8_t *data, uint16_t length); // Augmented CCITT
uint16_t xy_crc16_buypass(uint8_t *data, uint16_t length);   // Buypass
uint16_t xy_crc16_dds_110(uint8_t *data,
                          uint16_t length); // Distributed Data Store
uint16_t xy_crc16_dect_r(uint8_t *data, uint16_t length);       // DECT-R
uint16_t xy_crc16_dect_x(uint8_t *data, uint16_t length);       // DECT-X
uint16_t xy_crc16_genibus(uint8_t *data, uint16_t length);      // Genibus/EBU
uint16_t xy_crc16_gsm(uint8_t *data, uint16_t length);          // GSM networks
uint16_t xy_crc16_cms(uint8_t *data, uint16_t length);          // CMS
uint16_t xy_crc16_en_13757(uint8_t *data, uint16_t length);     // EN-13757
uint16_t xy_crc16_lj1200(uint8_t *data, uint16_t length);       // LJ1200
uint16_t xy_crc16_opensafety_a(uint8_t *data, uint16_t length); // OpenSAFETY-A
uint16_t xy_crc16_opensafety_b(uint8_t *data, uint16_t length); // OpenSAFETY-B
uint16_t xy_crc16_riello(uint8_t *data, uint16_t length);       // Riello
uint16_t xy_crc16_t10_dif(uint8_t *data, uint16_t length);      // T10-DIF
uint16_t xy_crc16_teledisk(uint8_t *data, uint16_t length);     // TeleDisk
uint16_t xy_crc16_tms37157(uint8_t *data, uint16_t length);     // TMS37157
uint16_t xy_crc16_a(uint8_t *data, uint16_t length);            // CRC-A
uint16_t xy_crc16_b(uint8_t *data, uint16_t length);            // CRC-B
uint16_t xy_crc16_cdma2000(uint8_t *data, uint16_t length);     // CDMA2000
uint16_t xy_crc16_dectr(uint8_t *data, uint16_t length);        // DECT-R
uint16_t xy_crc16_dectx(uint8_t *data, uint16_t length);        // DECT-X
uint16_t xy_crc16_epc(uint8_t *data, uint16_t length);          // EPC
uint16_t xy_crc16_epc_c1g2(uint8_t *data, uint16_t length);     // EPC C1G2
uint16_t xy_crc16_flexray(uint8_t *data, uint16_t length);      // FlexRay
uint16_t xy_crc16_kermit(uint8_t *data, uint16_t length);       // Kermit
uint16_t xy_crc16_m17(uint8_t *data, uint16_t length);          // M17
uint16_t xy_crc16_modbus_arc(uint8_t *data, uint16_t length);   // Modbus/ARC
uint16_t xy_crc16_nrsc_5(uint8_t *data, uint16_t length);       // NRSC-5
uint16_t xy_crc16_opensafety(uint8_t *data, uint16_t length);   // OpenSAFETY
uint16_t xy_crc16_profibus_arc(uint8_t *data, uint16_t length); // Profibus/ARC
uint16_t xy_crc16_umts(uint8_t *data, uint16_t length);         // UMTS

// CRC32 variants
uint32_t xy_crc32_normal(uint8_t *data, uint16_t length);     // Standard CRC32
uint32_t xy_crc32_mpeg2(uint8_t *data, uint16_t length);      // For MPEG2
uint32_t xy_crc32_bzip2(uint8_t *data, uint16_t length);      // BZIP2
uint32_t xy_crc32_jamcrc(uint8_t *data, uint16_t length);     // JAMCRC
uint32_t xy_crc32_c(uint8_t *data, uint16_t length);          // Castagnoli
uint32_t xy_crc32_d(uint8_t *data, uint16_t length);          // D polynomial
uint32_t xy_crc32_posix(uint8_t *data, uint16_t length);      // POSIX cksum
uint32_t xy_crc32_autosar(uint8_t *data, uint16_t length);    // Automotive
uint32_t xy_crc32_base91d(uint8_t *data, uint16_t length);    // Base91-D
uint32_t xy_crc32_cd_rom_edc(uint8_t *data, uint16_t length); // CD-ROM EDC
uint32_t xy_crc32_iscsi(uint8_t *data, uint16_t length);      // iSCSI
uint32_t xy_crc32_aixm(uint8_t *data, uint16_t length);       // AIXM
uint32_t xy_crc32_cksum(uint8_t *data, uint16_t length);      // CKSUM
uint32_t xy_crc32_iso_hdlc(uint8_t *data, uint16_t length);   // ISO-HDLC
uint32_t xy_crc32_xfer(uint8_t *data, uint16_t length);       // XFER

// CRC64 variants
uint64_t xy_crc64_ecma(uint8_t *data, uint16_t length);    // ECMA-182
uint64_t xy_crc64_iso(uint8_t *data, uint16_t length);     // ISO 3309
uint64_t xy_crc64_we(uint8_t *data, uint16_t length);      // Wolfgang Erhardt
uint64_t xy_crc64_go_iso(uint8_t *data, uint16_t length);  // GO-ISO
uint64_t xy_crc64_ms(uint8_t *data, uint16_t length);      // Microsoft
uint64_t xy_crc64_redis(uint8_t *data, uint16_t length);   // Redis DB
uint64_t xy_crc64_xz(uint8_t *data, uint16_t length);      // XZ compression
uint64_t xy_crc64_jones(uint8_t *data, uint16_t length);   // Jones
uint64_t xy_crc64_go_ecma(uint8_t *data, uint16_t length); // GO-ECMA

#ifdef __cplusplus
}
#endif

#endif /* __XY_CRC_H__ */