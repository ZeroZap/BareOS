/**
 * @file xy_iso7816.c
 * @brief ISO7816 Protocol Implementation
 * @version 1.0
 * @date 2025-11-01
 */

#include "xy_iso7816.h"
#include "../../xy_clib/xy_string.h"
#include <string.h>

/* ========================================================================== */
/* Private Helper Functions */
/* ========================================================================== */

/**
 * @brief Calculate XOR checksum for T=0 protocol
 */
static xy_u8 xy_iso7816_calc_checksum(const xy_u8 *data, xy_u16 len) {
    xy_u8 checksum = 0;
    for (xy_u16 i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

/**
 * @brief Send single byte
 */
static xy_iso7816_error_t xy_iso7816_send_byte(xy_iso7816_handle_t *handle, xy_u8 byte) {
    int ret = xy_hal_uart_send(handle->uart, &byte, 1, handle->timeout);
    if (ret != 1) {
        return XY_ISO7816_ERROR_IO;
    }
    return XY_ISO7816_OK;
}

/**
 * @brief Receive single byte
 */
static xy_iso7816_error_t xy_iso7816_recv_byte(xy_iso7816_handle_t *handle, xy_u8 *byte) {
    int ret = xy_hal_uart_recv(handle->uart, byte, 1, XY_ISO7816_BYTE_TIMEOUT);
    if (ret != 1) {
        return XY_ISO7816_ERROR_TIMEOUT;
    }
    return XY_ISO7816_OK;
}

/**
 * @brief Send buffer
 */
static xy_iso7816_error_t xy_iso7816_send_buffer(xy_iso7816_handle_t *handle,
                                                 const xy_u8 *data,
                                                 xy_u16 len) {
    int ret = xy_hal_uart_send(handle->uart, data, len, handle->timeout);
    if (ret != (int)len) {
        return XY_ISO7816_ERROR_IO;
    }
    return XY_ISO7816_OK;
}

/**
 * @brief Receive buffer
 */
static xy_iso7816_error_t xy_iso7816_recv_buffer(xy_iso7816_handle_t *handle,
                                                 xy_u8 *data,
                                                 xy_u16 len) {
    int ret = xy_hal_uart_recv(handle->uart, data, len, handle->timeout);
    if (ret != (int)len) {
        return XY_ISO7816_ERROR_TIMEOUT;
    }
    return XY_ISO7816_OK;
}

/* ========================================================================== */
/* Core Protocol Functions */
/* ========================================================================== */

xy_iso7816_error_t xy_iso7816_init(xy_iso7816_handle_t *handle, void *uart) {
    if (!handle || !uart) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    memset(handle, 0, sizeof(xy_iso7816_handle_t));
    handle->uart = uart;
    handle->timeout = XY_ISO7816_DEFAULT_TIMEOUT;
    handle->initialized = true;

    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_deinit(xy_iso7816_handle_t *handle) {
    if (!handle) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    handle->initialized = false;
    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_reset(xy_iso7816_handle_t *handle, xy_iso7816_atr_t *atr) {
    if (!handle || !handle->initialized || !atr) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_error_t ret;
    memset(atr, 0, sizeof(xy_iso7816_atr_t));

    /* Flush any pending data */
    xy_hal_uart_flush(handle->uart);

    /* Wait for initial character (TS - Initial Character) */
    xy_u8 ts;
    ret = xy_iso7816_recv_byte(handle, &ts);
    if (ret != XY_ISO7816_OK) {
        return XY_ISO7816_ERROR_ATR;
    }

    /* Validate TS (should be 0x3B for direct convention or 0x3F for inverse) */
    if (ts != 0x3B && ts != 0x3F) {
        return XY_ISO7816_ERROR_ATR;
    }

    atr->data[0] = ts;
    atr->length = 1;

    /* Receive T0 (Format Character) */
    xy_u8 t0;
    ret = xy_iso7816_recv_byte(handle, &t0);
    if (ret != XY_ISO7816_OK) {
        return XY_ISO7816_ERROR_ATR;
    }

    atr->data[atr->length++] = t0;

    /* Extract number of historical bytes */
    xy_u8 hist_bytes = t0 & 0x0F;
    xy_u8 td = t0;

    /* Read interface bytes (TA, TB, TC, TD) */
    while (atr->length < XY_ISO7816_ATR_MAX_LEN) {
        xy_u8 has_ta = (td & 0x10) != 0;
        xy_u8 has_tb = (td & 0x20) != 0;
        xy_u8 has_tc = (td & 0x40) != 0;
        xy_u8 has_td = (td & 0x80) != 0;

        if (has_ta) {
            ret = xy_iso7816_recv_byte(handle, &atr->data[atr->length++]);
            if (ret != XY_ISO7816_OK) return XY_ISO7816_ERROR_ATR;
        }

        if (has_tb) {
            ret = xy_iso7816_recv_byte(handle, &atr->data[atr->length++]);
            if (ret != XY_ISO7816_OK) return XY_ISO7816_ERROR_ATR;
        }

        if (has_tc) {
            ret = xy_iso7816_recv_byte(handle, &atr->data[atr->length++]);
            if (ret != XY_ISO7816_OK) return XY_ISO7816_ERROR_ATR;
        }

        if (has_td) {
            ret = xy_iso7816_recv_byte(handle, &td);
            if (ret != XY_ISO7816_OK) return XY_ISO7816_ERROR_ATR;
            atr->data[atr->length++] = td;
        } else {
            break;
        }
    }

    /* Read historical bytes */
    for (xy_u8 i = 0; i < hist_bytes && atr->length < XY_ISO7816_ATR_MAX_LEN; i++) {
        ret = xy_iso7816_recv_byte(handle, &atr->data[atr->length++]);
        if (ret != XY_ISO7816_OK) return XY_ISO7816_ERROR_ATR;
    }

    /* Read TCK (checksum) if T=1 protocol is used */
    if ((t0 & 0x80) && (td & 0x0F) != 0) {
        xy_u8 tck;
        ret = xy_iso7816_recv_byte(handle, &tck);
        if (ret != XY_ISO7816_OK) return XY_ISO7816_ERROR_ATR;
        atr->data[atr->length++] = tck;
    }

    atr->valid = true;
    atr->protocol = td & 0x0F; /* T=0 or T=1 */

    /* Store ATR in handle */
    memcpy(&handle->atr, atr, sizeof(xy_iso7816_atr_t));

    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_transceive(xy_iso7816_handle_t *handle,
                                         const xy_iso7816_apdu_cmd_t *cmd,
                                         xy_iso7816_apdu_resp_t *resp) {
    if (!handle || !handle->initialized || !cmd || !resp) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_error_t ret;
    memset(resp, 0, sizeof(xy_iso7816_apdu_resp_t));

    /* Send APDU header (CLA INS P1 P2) */
    xy_u8 header[5];
    header[0] = cmd->cla;
    header[1] = cmd->ins;
    header[2] = cmd->p1;
    header[3] = cmd->p2;
    header[4] = cmd->lc;

    ret = xy_iso7816_send_buffer(handle, header, 5);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Wait for procedure byte */
    xy_u8 proc_byte;
    ret = xy_iso7816_recv_byte(handle, &proc_byte);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Handle procedure byte */
    if (proc_byte == cmd->ins) {
        /* ACK - Send data if present */
        if (cmd->lc > 0) {
            ret = xy_iso7816_send_buffer(handle, cmd->data, cmd->lc);
            if (ret != XY_ISO7816_OK) {
                return ret;
            }
        }
    } else if (proc_byte == (xy_u8)(~cmd->ins)) {
        /* ACK complement - Send data byte by byte */
        for (xy_u8 i = 0; i < cmd->lc; i++) {
            ret = xy_iso7816_send_byte(handle, cmd->data[i]);
            if (ret != XY_ISO7816_OK) {
                return ret;
            }
            /* Wait for ACK for each byte */
            ret = xy_iso7816_recv_byte(handle, &proc_byte);
            if (ret != XY_ISO7816_OK) {
                return ret;
            }
        }
    } else if (proc_byte == 0x60 || proc_byte == 0x61) {
        /* NULL or more data - handled below */
    } else {
        /* Error or invalid procedure byte */
        return XY_ISO7816_ERROR_PROTOCOL;
    }

    /* Receive response data if expected */
    if (cmd->le > 0) {
        xy_u8 to_receive = (cmd->le == 0) ? 0 : cmd->le;
        
        while (resp->length < to_receive && resp->length < 256) {
            ret = xy_iso7816_recv_byte(handle, &resp->data[resp->length]);
            if (ret != XY_ISO7816_OK) {
                break;
            }
            resp->length++;
        }
    }

    /* Receive status words SW1 and SW2 */
    ret = xy_iso7816_recv_byte(handle, &resp->sw1);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    ret = xy_iso7816_recv_byte(handle, &resp->sw2);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Handle 0x61XX - more data available */
    if (resp->sw1 == 0x61) {
        xy_iso7816_apdu_cmd_t get_resp;
        memset(&get_resp, 0, sizeof(get_resp));
        get_resp.cla = cmd->cla;
        get_resp.ins = XY_ISO7816_INS_GET_RESPONSE;
        get_resp.p1 = 0x00;
        get_resp.p2 = 0x00;
        get_resp.lc = 0;
        get_resp.le = resp->sw2;

        xy_iso7816_apdu_resp_t more_resp;
        ret = xy_iso7816_transceive(handle, &get_resp, &more_resp);
        if (ret == XY_ISO7816_OK) {
            /* Append additional data */
            xy_u16 copy_len = more_resp.length;
            if (resp->length + copy_len > 256) {
                copy_len = 256 - resp->length;
            }
            memcpy(&resp->data[resp->length], more_resp.data, copy_len);
            resp->length += copy_len;
            resp->sw1 = more_resp.sw1;
            resp->sw2 = more_resp.sw2;
        }
    }

    return XY_ISO7816_OK;
}

xy_bool xy_iso7816_is_success(const xy_iso7816_apdu_resp_t *resp) {
    if (!resp) {
        return false;
    }
    return (resp->sw1 == 0x90 && resp->sw2 == 0x00);
}

xy_u16 xy_iso7816_get_sw(const xy_iso7816_apdu_resp_t *resp) {
    if (!resp) {
        return 0;
    }
    return (xy_u16)((resp->sw1 << 8) | resp->sw2);
}

/* ========================================================================== */
/* SIM Card Operations */
/* ========================================================================== */

xy_iso7816_error_t xy_iso7816_select_file(xy_iso7816_handle_t *handle, xy_u16 file_id) {
    if (!handle || !handle->initialized) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = XY_ISO7816_INS_SELECT;
    cmd.p1 = 0x00;  /* Select by file ID */
    cmd.p2 = 0x04;  /* Return FCP template */
    cmd.lc = 2;
    cmd.data[0] = (xy_u8)(file_id >> 8);
    cmd.data[1] = (xy_u8)(file_id & 0xFF);
    cmd.le = 0;

    xy_iso7816_error_t ret = xy_iso7816_transceive(handle, &cmd, &resp);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    if (!xy_iso7816_is_success(&resp)) {
        return XY_ISO7816_ERROR_CARD;
    }

    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_read_binary(xy_iso7816_handle_t *handle,
                                          xy_u16 offset,
                                          xy_u8 *data,
                                          xy_u8 len) {
    if (!handle || !handle->initialized || !data || len == 0) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = XY_ISO7816_INS_READ_BINARY;
    cmd.p1 = (xy_u8)(offset >> 8);
    cmd.p2 = (xy_u8)(offset & 0xFF);
    cmd.lc = 0;
    cmd.le = len;

    xy_iso7816_error_t ret = xy_iso7816_transceive(handle, &cmd, &resp);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    if (!xy_iso7816_is_success(&resp)) {
        return XY_ISO7816_ERROR_CARD;
    }

    memcpy(data, resp.data, (resp.length < len) ? resp.length : len);
    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_detect_card_type(xy_iso7816_handle_t *handle,
                                               xy_iso7816_card_type_t *card_type) {
    if (!handle || !handle->initialized || !card_type) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    *card_type = XY_ISO7816_CARD_UNKNOWN;

    /* Try to select USIM application (AID) */
    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = 0x00;
    cmd.ins = XY_ISO7816_INS_SELECT;
    cmd.p1 = 0x04;  /* Select by AID */
    cmd.p2 = 0x00;

    /* USIM AID: A0 00 00 00 87 10 02 */
    const xy_u8 usim_aid[] = {0xA0, 0x00, 0x00, 0x00, 0x87, 0x10, 0x02};
    cmd.lc = sizeof(usim_aid);
    memcpy(cmd.data, usim_aid, cmd.lc);
    cmd.le = 0;

    xy_iso7816_error_t ret = xy_iso7816_transceive(handle, &cmd, &resp);
    if (ret == XY_ISO7816_OK && xy_iso7816_is_success(&resp)) {
        *card_type = XY_ISO7816_CARD_USIM;
        return XY_ISO7816_OK;
    }

    /* Try to select GSM MF (for SIM cards) */
    ret = xy_iso7816_select_file(handle, XY_ISO7816_FID_MF);
    if (ret == XY_ISO7816_OK) {
        *card_type = XY_ISO7816_CARD_SIM;
        return XY_ISO7816_OK;
    }

    *card_type = XY_ISO7816_CARD_GENERIC;
    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_read_iccid(xy_iso7816_handle_t *handle,
                                         xy_u8 *iccid, xy_u8 *len) {
    if (!handle || !handle->initialized || !iccid || !len) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_error_t ret;

    /* Select MF */
    ret = xy_iso7816_select_file(handle, XY_ISO7816_FID_MF);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Select EF_ICCID */
    ret = xy_iso7816_select_file(handle, XY_ISO7816_FID_EF_ICCID);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Read ICCID (10 bytes BCD) */
    xy_u8 buffer[10];
    ret = xy_iso7816_read_binary(handle, 0, buffer, 10);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    memcpy(iccid, buffer, 10);
    *len = 10;

    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_read_imsi(xy_iso7816_handle_t *handle,
                                        xy_u8 *imsi, xy_u8 *len) {
    if (!handle || !handle->initialized || !imsi || !len) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_error_t ret;

    /* Select MF */
    ret = xy_iso7816_select_file(handle, XY_ISO7816_FID_MF);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Select DF_GSM */
    ret = xy_iso7816_select_file(handle, XY_ISO7816_FID_DF_GSM);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Select EF_IMSI */
    ret = xy_iso7816_select_file(handle, XY_ISO7816_FID_EF_IMSI);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Read IMSI (9 bytes: 1 length + 8 BCD) */
    xy_u8 buffer[9];
    ret = xy_iso7816_read_binary(handle, 0, buffer, 9);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    memcpy(imsi, buffer, 9);
    *len = buffer[0]; /* First byte is length */

    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_get_sim_info(xy_iso7816_handle_t *handle,
                                           xy_iso7816_sim_info_t *info) {
    if (!handle || !handle->initialized || !info) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    memset(info, 0, sizeof(xy_iso7816_sim_info_t));

    /* Detect card type */
    xy_iso7816_error_t ret = xy_iso7816_detect_card_type(handle, &info->card_type);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    /* Read ICCID */
    ret = xy_iso7816_read_iccid(handle, info->iccid, &info->iccid_len);
    if (ret != XY_ISO7816_OK) {
        /* Non-fatal, continue */
    }

    /* Read IMSI */
    ret = xy_iso7816_read_imsi(handle, info->imsi, &info->imsi_len);
    if (ret != XY_ISO7816_OK) {
        /* Non-fatal, continue */
    }

    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_verify_pin(xy_iso7816_handle_t *handle,
                                         const char *pin,
                                         xy_u8 *remaining_tries) {
    if (!handle || !handle->initialized || !pin) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_u8 pin_len = 0;
    while (pin[pin_len] != '\0' && pin_len < 8) {
        pin_len++;
    }

    if (pin_len < 4 || pin_len > 8) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = XY_ISO7816_INS_VERIFY_PIN;
    cmd.p1 = 0x00;
    cmd.p2 = 0x01;  /* CHV1 (PIN1) */
    cmd.lc = 8;

    /* Copy PIN and pad with 0xFF */
    for (xy_u8 i = 0; i < 8; i++) {
        if (i < pin_len) {
            cmd.data[i] = (xy_u8)pin[i];
        } else {
            cmd.data[i] = 0xFF;
        }
    }

    cmd.le = 0;

    xy_iso7816_error_t ret = xy_iso7816_transceive(handle, &cmd, &resp);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    if (xy_iso7816_is_success(&resp)) {
        if (remaining_tries) {
            *remaining_tries = 0;
        }
        return XY_ISO7816_OK;
    }

    /* Extract remaining tries from SW2 if SW1=0x63 */
    if (resp.sw1 == 0x63 && (resp.sw2 & 0xF0) == 0xC0) {
        if (remaining_tries) {
            *remaining_tries = resp.sw2 & 0x0F;
        }
    }

    return XY_ISO7816_ERROR_CARD;
}

xy_iso7816_error_t xy_iso7816_get_challenge(xy_iso7816_handle_t *handle, xy_u8 *rand) {
    if (!handle || !handle->initialized || !rand) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = XY_ISO7816_INS_GET_CHALLENGE;
    cmd.p1 = 0x00;
    cmd.p2 = 0x00;
    cmd.lc = 0;
    cmd.le = 16;  /* Request 16 bytes */

    xy_iso7816_error_t ret = xy_iso7816_transceive(handle, &cmd, &resp);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    if (!xy_iso7816_is_success(&resp) || resp.length < 16) {
        return XY_ISO7816_ERROR_CARD;
    }

    memcpy(rand, resp.data, 16);
    return XY_ISO7816_OK;
}

xy_iso7816_error_t xy_iso7816_authenticate(xy_iso7816_handle_t *handle,
                                           const xy_u8 *rand,
                                           const xy_u8 *autn,
                                           xy_u8 *res,
                                           xy_u8 *ck,
                                           xy_u8 *ik) {
    if (!handle || !handle->initialized || !rand || !autn || !res || !ck || !ik) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;

    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = 0x00;
    cmd.ins = XY_ISO7816_INS_AUTHENTICATE;
    cmd.p1 = 0x00;
    cmd.p2 = 0x80;  /* 3G authentication context */
    cmd.lc = 33;    /* 1 (tag) + 16 (RAND) + 1 (tag) + 16 (AUTN) */

    /* Build authentication data: TAG_RAND (0x10) + RAND + TAG_AUTN (0x11) + AUTN */
    cmd.data[0] = 0x10;  /* RAND tag */
    memcpy(&cmd.data[1], rand, 16);
    cmd.data[17] = 0x11;  /* AUTN tag */
    memcpy(&cmd.data[18], autn, 16);

    cmd.le = 0;

    xy_iso7816_error_t ret = xy_iso7816_transceive(handle, &cmd, &resp);
    if (ret != XY_ISO7816_OK) {
        return ret;
    }

    if (!xy_iso7816_is_success(&resp)) {
        return XY_ISO7816_ERROR_CARD;
    }

    /* Parse response: TAG_RES + RES + TAG_CK + CK + TAG_IK + IK */
    xy_u16 idx = 0;
    
    if (resp.data[idx++] != 0xDB) {  /* Response tag */
        return XY_ISO7816_ERROR_PROTOCOL;
    }

    xy_u8 resp_len = resp.data[idx++];
    
    /* Extract RES (typically 8 bytes) */
    if (idx < resp.length && resp.data[idx] == 0x80) {
        idx++;  /* RES tag */
        xy_u8 res_len = resp.data[idx++];
        if (res_len <= 8) {
            memcpy(res, &resp.data[idx], res_len);
            idx += res_len;
        }
    }

    /* Extract CK (16 bytes) */
    if (idx < resp.length && resp.data[idx] == 0x81) {
        idx++;  /* CK tag */
        xy_u8 ck_len = resp.data[idx++];
        if (ck_len == 16) {
            memcpy(ck, &resp.data[idx], 16);
            idx += 16;
        }
    }

    /* Extract IK (16 bytes) */
    if (idx < resp.length && resp.data[idx] == 0x82) {
        idx++;  /* IK tag */
        xy_u8 ik_len = resp.data[idx++];
        if (ik_len == 16) {
            memcpy(ik, &resp.data[idx], 16);
        }
    }

    return XY_ISO7816_OK;
}

/* ========================================================================== */
/* Utility Functions */
/* ========================================================================== */

xy_u8 xy_iso7816_bcd_to_ascii(const xy_u8 *bcd, xy_u8 bcd_len,
                              char *ascii, xy_u8 ascii_len) {
    if (!bcd || !ascii || ascii_len == 0) {
        return 0;
    }

    xy_u8 idx = 0;
    for (xy_u8 i = 0; i < bcd_len && idx < ascii_len - 1; i++) {
        xy_u8 low = bcd[i] & 0x0F;
        xy_u8 high = (bcd[i] >> 4) & 0x0F;

        /* Swap nibbles for ICCID/IMSI encoding */
        if (low <= 9) {
            ascii[idx++] = '0' + low;
        }
        if (idx >= ascii_len - 1) break;

        if (high <= 9) {
            ascii[idx++] = '0' + high;
        }
    }

    ascii[idx] = '\0';
    return idx;
}

xy_iso7816_error_t xy_iso7816_parse_atr(xy_iso7816_atr_t *atr) {
    if (!atr || !atr->valid) {
        return XY_ISO7816_ERROR_INVALID_PARAM;
    }

    /* ATR parsing is already done in xy_iso7816_reset */
    /* This function can be extended for detailed ATR analysis */

    return XY_ISO7816_OK;
}
