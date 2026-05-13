/**
 * @file xy_iso7816.h
 * @brief ISO7816 Protocol Implementation (Smart Card / SIM Card Communication)
 * @version 1.0
 * @date 2025-11-01
 * 
 * This module implements ISO7816-3 T=0 protocol for smart card communication
 * using the XY HAL UART interface.
 */

#ifndef XY_ISO7816_H
#define XY_ISO7816_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../xy_clib/xy_typedef.h"
#include "../../../bsp/xy_hal/inc/xy_hal_uart.h"

/* ISO7816 Protocol Constants */
#define XY_ISO7816_ATR_MAX_LEN          33    /**< Maximum ATR length */
#define XY_ISO7816_APDU_MAX_LEN         261   /**< Maximum APDU length (CLA+INS+P1+P2+Lc+Data+Le) */
#define XY_ISO7816_RESPONSE_MAX_LEN     258   /**< Maximum response length (256 data + 2 SW) */

/* ISO7816 Timing Constants (ETU - Elementary Time Unit based) */
#define XY_ISO7816_DEFAULT_TIMEOUT      1000  /**< Default timeout in ms */
#define XY_ISO7816_ATR_TIMEOUT          20000 /**< ATR timeout in ms (ISO7816-3: 20s) */
#define XY_ISO7816_BYTE_TIMEOUT         100   /**< Byte-to-byte timeout in ms */

/* ISO7816 Status Words */
#define XY_ISO7816_SW_SUCCESS           0x9000 /**< Normal completion */
#define XY_ISO7816_SW_MORE_DATA         0x61FF /**< More data available (0x61XX) */
#define XY_ISO7816_SW_WRONG_LENGTH      0x6700 /**< Wrong length */
#define XY_ISO7816_SW_SECURITY_STATUS   0x6982 /**< Security status not satisfied */
#define XY_ISO7816_SW_AUTH_BLOCKED      0x6983 /**< Authentication method blocked */
#define XY_ISO7816_SW_CONDITIONS        0x6985 /**< Conditions not satisfied */
#define XY_ISO7816_SW_WRONG_PARAMS      0x6A86 /**< Incorrect parameters P1-P2 */
#define XY_ISO7816_SW_FILE_NOT_FOUND    0x6A82 /**< File not found */
#define XY_ISO7816_SW_INS_NOT_SUPPORTED 0x6D00 /**< Instruction not supported */
#define XY_ISO7816_SW_CLA_NOT_SUPPORTED 0x6E00 /**< Class not supported */
#define XY_ISO7816_SW_WRONG_PIN         0x63C0 /**< Wrong PIN (0x63CX, X=remaining tries) */

/* APDU Class Byte (CLA) */
#define XY_ISO7816_CLA_DEFAULT          0x00  /**< Default class */
#define XY_ISO7816_CLA_GSM              0xA0  /**< GSM class for SIM cards */

/* APDU Instruction Byte (INS) */
#define XY_ISO7816_INS_SELECT           0xA4  /**< SELECT FILE */
#define XY_ISO7816_INS_READ_BINARY      0xB0  /**< READ BINARY */
#define XY_ISO7816_INS_READ_RECORD      0xB2  /**< READ RECORD */
#define XY_ISO7816_INS_UPDATE_BINARY    0xD6  /**< UPDATE BINARY */
#define XY_ISO7816_INS_UPDATE_RECORD    0xDC  /**< UPDATE RECORD */
#define XY_ISO7816_INS_GET_RESPONSE     0xC0  /**< GET RESPONSE */
#define XY_ISO7816_INS_VERIFY_PIN       0x20  /**< VERIFY CHV (PIN) */
#define XY_ISO7816_INS_GET_CHALLENGE    0x84  /**< GET CHALLENGE (for authentication) */
#define XY_ISO7816_INS_AUTHENTICATE     0x88  /**< AUTHENTICATE */

/* Common File IDs for SIM Cards (ETSI TS 102.221) */
#define XY_ISO7816_FID_MF               0x3F00 /**< Master File */
#define XY_ISO7816_FID_DF_TELECOM       0x7F10 /**< Telecom DF */
#define XY_ISO7816_FID_DF_GSM           0x7F20 /**< GSM DF */
#define XY_ISO7816_FID_EF_ICCID         0x2FE2 /**< ICCID (SIM card ID) */
#define XY_ISO7816_FID_EF_IMSI          0x6F07 /**< IMSI */
#define XY_ISO7816_FID_EF_LOCI          0x6F7E /**< Location Information */
#define XY_ISO7816_FID_EF_AD            0x6FAD /**< Administrative Data */
#define XY_ISO7816_FID_EF_SPN           0x6F46 /**< Service Provider Name */

/* ISO7816 Error Codes */
typedef enum {
    XY_ISO7816_OK                  = 0,   /**< Success */
    XY_ISO7816_ERROR               = -1,  /**< Generic error */
    XY_ISO7816_ERROR_INVALID_PARAM = -2,  /**< Invalid parameter */
    XY_ISO7816_ERROR_TIMEOUT       = -3,  /**< Communication timeout */
    XY_ISO7816_ERROR_IO            = -4,  /**< I/O error */
    XY_ISO7816_ERROR_PROTOCOL      = -5,  /**< Protocol error */
    XY_ISO7816_ERROR_ATR           = -6,  /**< ATR error */
    XY_ISO7816_ERROR_NOT_INIT      = -7,  /**< Not initialized */
    XY_ISO7816_ERROR_CARD          = -8,  /**< Card error (from SW1/SW2) */
} xy_iso7816_error_t;

/* Card Type Detection */
typedef enum {
    XY_ISO7816_CARD_UNKNOWN = 0,
    XY_ISO7816_CARD_SIM,        /**< 2G SIM */
    XY_ISO7816_CARD_USIM,       /**< 3G USIM */
    XY_ISO7816_CARD_ISIM,       /**< IMS ISIM */
    XY_ISO7816_CARD_GENERIC,    /**< Generic ISO7816 card */
} xy_iso7816_card_type_t;

/**
 * @brief ATR (Answer To Reset) Structure
 */
typedef struct {
    xy_u8 data[XY_ISO7816_ATR_MAX_LEN]; /**< Raw ATR bytes */
    xy_u8 length;                        /**< ATR length */
    xy_u8 protocol;                      /**< Protocol type (T=0, T=1) */
    xy_bool valid;                       /**< ATR validity flag */
} xy_iso7816_atr_t;

/**
 * @brief APDU Command Structure (Command APDU)
 */
typedef struct {
    xy_u8 cla;                           /**< Class byte */
    xy_u8 ins;                           /**< Instruction byte */
    xy_u8 p1;                            /**< Parameter 1 */
    xy_u8 p2;                            /**< Parameter 2 */
    xy_u8 lc;                            /**< Data length (0 if no data) */
    xy_u8 data[256];                     /**< Command data */
    xy_u8 le;                            /**< Expected response length (0 = 256) */
} xy_iso7816_apdu_cmd_t;

/**
 * @brief APDU Response Structure (Response APDU)
 */
typedef struct {
    xy_u8 data[256];                     /**< Response data */
    xy_u16 length;                       /**< Response data length */
    xy_u8 sw1;                           /**< Status word 1 */
    xy_u8 sw2;                           /**< Status word 2 */
} xy_iso7816_apdu_resp_t;

/**
 * @brief ISO7816 Interface Handle
 */
typedef struct {
    void *uart;                          /**< UART handle */
    xy_iso7816_atr_t atr;               /**< ATR information */
    xy_bool initialized;                 /**< Initialization flag */
    xy_u32 timeout;                      /**< Default timeout in ms */
} xy_iso7816_handle_t;

/**
 * @brief SIM Card Information Structure
 */
typedef struct {
    xy_iso7816_card_type_t card_type;    /**< Card type */
    xy_u8 iccid[10];                     /**< ICCID (20 BCD digits -> 10 bytes) */
    xy_u8 iccid_len;                     /**< ICCID length */
    xy_u8 imsi[9];                       /**< IMSI (15 BCD digits -> 9 bytes) */
    xy_u8 imsi_len;                      /**< IMSI length */
} xy_iso7816_sim_info_t;

/* ========================================================================== */
/* Core Protocol Functions */
/* ========================================================================== */

/**
 * @brief Initialize ISO7816 interface
 * @param handle ISO7816 handle
 * @param uart UART instance (must be pre-configured)
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_init(xy_iso7816_handle_t *handle, void *uart);

/**
 * @brief Deinitialize ISO7816 interface
 * @param handle ISO7816 handle
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_deinit(xy_iso7816_handle_t *handle);

/**
 * @brief Perform card reset and get ATR (Answer To Reset)
 * @param handle ISO7816 handle
 * @param atr Pointer to store ATR data
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_reset(xy_iso7816_handle_t *handle, xy_iso7816_atr_t *atr);

/**
 * @brief Send APDU command and receive response
 * @param handle ISO7816 handle
 * @param cmd APDU command
 * @param resp APDU response
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_transceive(xy_iso7816_handle_t *handle,
                                         const xy_iso7816_apdu_cmd_t *cmd,
                                         xy_iso7816_apdu_resp_t *resp);

/**
 * @brief Check if last response indicates success
 * @param resp APDU response
 * @return true if SW=0x9000
 */
xy_bool xy_iso7816_is_success(const xy_iso7816_apdu_resp_t *resp);

/**
 * @brief Get status word from response
 * @param resp APDU response
 * @return 16-bit status word (SW1 << 8 | SW2)
 */
xy_u16 xy_iso7816_get_sw(const xy_iso7816_apdu_resp_t *resp);

/* ========================================================================== */
/* SIM Card Operations */
/* ========================================================================== */

/**
 * @brief Detect SIM card type
 * @param handle ISO7816 handle
 * @param card_type Pointer to store detected card type
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_detect_card_type(xy_iso7816_handle_t *handle,
                                               xy_iso7816_card_type_t *card_type);

/**
 * @brief Read ICCID (Integrated Circuit Card Identifier)
 * @param handle ISO7816 handle
 * @param iccid Buffer to store ICCID (min 10 bytes)
 * @param len Pointer to store ICCID length
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_read_iccid(xy_iso7816_handle_t *handle,
                                         xy_u8 *iccid, xy_u8 *len);

/**
 * @brief Read IMSI (International Mobile Subscriber Identity)
 * @param handle ISO7816 handle
 * @param imsi Buffer to store IMSI (min 9 bytes)
 * @param len Pointer to store IMSI length
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_read_imsi(xy_iso7816_handle_t *handle,
                                        xy_u8 *imsi, xy_u8 *len);

/**
 * @brief Get complete SIM card information
 * @param handle ISO7816 handle
 * @param info Pointer to store SIM card information
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_get_sim_info(xy_iso7816_handle_t *handle,
                                           xy_iso7816_sim_info_t *info);

/**
 * @brief Verify PIN (CHV1)
 * @param handle ISO7816 handle
 * @param pin PIN code (4-8 digits, null-terminated string)
 * @param remaining_tries Pointer to store remaining tries (can be NULL)
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_verify_pin(xy_iso7816_handle_t *handle,
                                         const char *pin,
                                         xy_u8 *remaining_tries);

/**
 * @brief Perform mutual authentication (3G/4G SIM)
 * @param handle ISO7816 handle
 * @param rand Random challenge (16 bytes)
 * @param autn Authentication token (16 bytes)
 * @param res Response (8 bytes output)
 * @param ck Cipher key (16 bytes output)
 * @param ik Integrity key (16 bytes output)
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_authenticate(xy_iso7816_handle_t *handle,
                                           const xy_u8 *rand,
                                           const xy_u8 *autn,
                                           xy_u8 *res,
                                           xy_u8 *ck,
                                           xy_u8 *ik);

/**
 * @brief Get authentication challenge (2G SIM)
 * @param handle ISO7816 handle
 * @param rand Random challenge output (16 bytes)
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_get_challenge(xy_iso7816_handle_t *handle,
                                            xy_u8 *rand);

/**
 * @brief Select file by file ID
 * @param handle ISO7816 handle
 * @param file_id File ID (e.g., 0x3F00 for MF)
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_select_file(xy_iso7816_handle_t *handle,
                                          xy_u16 file_id);

/**
 * @brief Read binary data from current file
 * @param handle ISO7816 handle
 * @param offset Offset to read from
 * @param data Buffer to store data
 * @param len Length to read
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_read_binary(xy_iso7816_handle_t *handle,
                                          xy_u16 offset,
                                          xy_u8 *data,
                                          xy_u8 len);

/* ========================================================================== */
/* Utility Functions */
/* ========================================================================== */

/**
 * @brief Convert BCD to ASCII string
 * @param bcd BCD data
 * @param bcd_len BCD length
 * @param ascii ASCII output buffer
 * @param ascii_len ASCII buffer size
 * @return Number of ASCII characters written
 */
xy_u8 xy_iso7816_bcd_to_ascii(const xy_u8 *bcd, xy_u8 bcd_len,
                              char *ascii, xy_u8 ascii_len);

/**
 * @brief Parse ATR and extract protocol information
 * @param atr ATR structure
 * @return XY_ISO7816_OK on success
 */
xy_iso7816_error_t xy_iso7816_parse_atr(xy_iso7816_atr_t *atr);

#ifdef __cplusplus
}
#endif

#endif /* XY_ISO7816_H */
