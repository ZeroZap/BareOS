#ifndef _XY_CRC_CFG_H_
#define _XY_CRC_CFG_H_

// Core feature configuration
#define XY_CRC_HW_SUPPORT    0 // Disable hardware support by default
#define XY_CRC_TABLE_SUPPORT 1 // Keep table support enabled

// CRC width support configuration
#define XY_CRC2_SUPPORT  1
#define XY_CRC3_SUPPORT  1
#define XY_CRC4_SUPPORT  1
#define XY_CRC5_SUPPORT  1
#define XY_CRC6_SUPPORT  1
#define XY_CRC7_SUPPORT  1
#define XY_CRC8_SUPPORT  1
#define XY_CRC16_SUPPORT 1
#define XY_CRC32_SUPPORT 1
#define XY_CRC64_SUPPORT 1

// CRC-2 variants
#if XY_CRC2_SUPPORT
#define XY_CRC2_G704_ENABLE 1
#define XY_CRC2_GSM_ENABLE  1
#endif

// CRC-3 variants
#if XY_CRC3_SUPPORT
#define XY_CRC3_ROHC_ENABLE 1
#define XY_CRC3_GSM_ENABLE  1
#endif

// CRC-4 variants
#if XY_CRC4_SUPPORT
#define XY_CRC4_ITU_ENABLE        1
#define XY_CRC4_INTERLAKEN_ENABLE 1
#endif

// CRC-5 variants
#if XY_CRC5_SUPPORT
#define XY_CRC5_EPC_ENABLE       1
#define XY_CRC5_ITU_ENABLE       1
#define XY_CRC5_USB_ENABLE       1
#define XY_CRC5_BLUETOOTH_ENABLE 1
#define XY_CRC5_DARC_ENABLE      1
#define XY_CRC5_GSM_ENABLE       1
#endif

// CRC-6 variants
#if XY_CRC6_SUPPORT
#define XY_CRC6_ITU_ENABLE       1
#define XY_CRC6_GSM_ENABLE       1
#define XY_CRC6_CDMA2000A_ENABLE 1
#define XY_CRC6_CDMA2000B_ENABLE 1
#define XY_CRC6_DARC_ENABLE      1
#define XY_CRC6_G704_ENABLE      1
#define XY_CRC6_ITU_K_ENABLE     1
#define XY_CRC6_ITU_O_ENABLE     1
#define XY_CRC6_ITU_Q_ENABLE     1
#define XY_CRC6_ITU_R_ENABLE     1
#define XY_CRC6_ITU_S_ENABLE     1
#endif

// CRC-7 variants
#if XY_CRC7_SUPPORT
#define XY_CRC7_MMC_ENABLE       1
#define XY_CRC7_ROHC_ENABLE      1
#define XY_CRC7_UMTS_ENABLE      1
#define XY_CRC7_BLUETOOTH_ENABLE 1
#define XY_CRC7_DARC_ENABLE      1
#define XY_CRC7_GSM_B_ENABLE     1
#endif

// CRC-8 variants
#if XY_CRC8_SUPPORT
#define XY_CRC8_NORMAL_ENABLE        1
#define XY_CRC8_MAXIM_ENABLE         1
#define XY_CRC8_ROHC_ENABLE          1
#define XY_CRC8_ITU_ENABLE           1
#define XY_CRC8_1WIRE_ENABLE         1
#define XY_CRC8_WCDMA_ENABLE         1
#define XY_CRC8_BLUETOOTH_ENABLE     1
#define XY_CRC8_AUTOSAR_ENABLE       1
#define XY_CRC8_DARC_ENABLE          1
#define XY_CRC8_DVB_S2_ENABLE        1
#define XY_CRC8_EBU_ENABLE           1
#define XY_CRC8_GSM_A_ENABLE         1
#define XY_CRC8_GSM_B_ENABLE         1
#define XY_CRC8_I_CODE_ENABLE        1
#define XY_CRC8_LTE_ENABLE           1
#define XY_CRC8_OPENSAFETY_ENABLE    1
#define XY_CRC8_SAE_J1850_ENABLE     1
#define XY_CRC8_AES_ENABLE           1
#define XY_CRC8_SAE_J1850_ZER_ENABLE 1
#define XY_CRC8_SAE_J1850_NON_ENABLE 1
#define XY_CRC8_NRSC_5_ENABLE        1
#define XY_CRC8_BLUETOOTH_HID_ENABLE 1
#define XY_CRC8_MIFARE_MAD_ENABLE    1
#endif

// CRC-16 variants
#if XY_CRC16_SUPPORT
#define XY_CRC16_CCITT_ENABLE        1
#define XY_CRC16_CCITT_FALSE_ENABLE  1
#define XY_CRC16_XMODEM_ENABLE       1
#define XY_CRC16_X25_ENABLE          1
#define XY_CRC16_MODBUS_ENABLE       1
#define XY_CRC16_IBM_ENABLE          1
#define XY_CRC16_MAXIM_ENABLE        1
#define XY_CRC16_USB_ENABLE          1
#define XY_CRC16_T10_DIF_ENABLE      1
#define XY_CRC16_DNP_ENABLE          1
#define XY_CRC16_DECT_ENABLE         1
#define XY_CRC16_EN_13757_ENABLE     1
#define XY_CRC16_GENIBUS_ENABLE      1
#define XY_CRC16_GSM_ENABLE          1
#define XY_CRC16_PROFIBUS_ARC_ENABLE 1
#define XY_CRC16_OPENSAFETY_A_ENABLE 1
#define XY_CRC16_OPENSAFETY_B_ENABLE 1
#define XY_CRC16_ARC_ENABLE          1
#define XY_CRC16_BUYPASS_ENABLE      1
#define XY_CRC16_DDS_110_ENABLE      1
#define XY_CRC16_TELEDISK_ENABLE     1
#define XY_CRC16_TMS37157_ENABLE     1
#define XY_CRC16_A_ENABLE            1
#define XY_CRC16_KERMIT_ENABLE       1
#define XY_CRC16_MCRF4XX_ENABLE      1
#define XY_CRC16_RIELLO_ENABLE       1
#define XY_CRC16_DECTX_ENABLE        1
#endif

// CRC-32 variants
#if XY_CRC32_SUPPORT
#define XY_CRC32_NORMAL_ENABLE     1
#define XY_CRC32_BZIP2_ENABLE      1
#define XY_CRC32_C_ENABLE          1
#define XY_CRC32_D_ENABLE          1
#define XY_CRC32_MPEG2_ENABLE      1
#define XY_CRC32_POSIX_ENABLE      1
#define XY_CRC32_JAMCRC_ENABLE     1
#define XY_CRC32_ISO_HDLC_ENABLE   1
#define XY_CRC32_AIXM_ENABLE       1
#define XY_CRC32_AUTOSAR_ENABLE    1
#define XY_CRC32_BASE91D_ENABLE    1
#define XY_CRC32_CKSUM_ENABLE      1
#define XY_CRC32_CD_ROM_EDC_ENABLE 1
#endif

// CRC-64 variants
#if XY_CRC64_SUPPORT
#define XY_CRC64_ECMA_ENABLE    1
#define XY_CRC64_GO_ECMA_ENABLE 1
#define XY_CRC64_GO_ISO_ENABLE  1
#define XY_CRC64_ISO_ENABLE     1
#define XY_CRC64_JONES_ENABLE   1
#define XY_CRC64_MS_ENABLE      1
#define XY_CRC64_REDIS_ENABLE   1
#define XY_CRC64_WE_ENABLE      1
#define XY_CRC64_XZ_ENABLE      1
#endif

// Basic configuration options
#define XY_CRC_RUNTIME_CONFIG 1   // Enable runtime configuration
#define XY_CRC_STATIC_TABLES  0   // Use static lookup tables
#define XY_CRC_OPTIMIZE_SPEED 1   // Optimize for speed over size
#define XY_CRC_TABLE_SIZE     256 // Size of lookup table

#endif /* _XY_CRC_CFG_H_ */