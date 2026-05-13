/**
 * @file xy_iso7816_example.c
 * @brief ISO7816 Usage Examples
 * @version 1.0
 * @date 2025-11-01
 * 
 * This file demonstrates how to use the ISO7816 protocol implementation
 * for SIM card communication.
 */

#include "xy_iso7816.h"
#include "../../../bsp/xy_hal/inc/xy_hal.h"
#include "../../xy_clib/xy_stdio.h"
#include <stdio.h>
#include <string.h>

/* ========================================================================== */
/* Example 1: Basic Initialization and ATR */
/* ========================================================================== */

void example_init_and_atr(void) {
    /* Assuming UART is already configured for ISO7816:
     * - Baudrate: 9600 bps (or as per card specification)
     * - 8 data bits, even parity, 2 stop bits
     */
    void *uart_handle = NULL; /* Platform-specific UART handle */
    
    /* Configure UART for ISO7816 */
    xy_hal_uart_config_t uart_config = {
        .baudrate = 9600,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_2,
        .parity = XY_HAL_UART_PARITY_EVEN,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    
    xy_hal_uart_init(uart_handle, &uart_config);
    
    /* Initialize ISO7816 interface */
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    ret = xy_iso7816_init(&iso_handle, uart_handle);
    if (ret != XY_ISO7816_OK) {
        printf("Failed to initialize ISO7816\n");
        return;
    }
    
    /* Perform card reset and get ATR */
    xy_iso7816_atr_t atr;
    ret = xy_iso7816_reset(&iso_handle, &atr);
    if (ret != XY_ISO7816_OK) {
        printf("Failed to get ATR\n");
        return;
    }
    
    /* Print ATR */
    printf("ATR received (%d bytes): ", atr.length);
    for (xy_u8 i = 0; i < atr.length; i++) {
        printf("%02X ", atr.data[i]);
    }
    printf("\nProtocol: T=%d\n", atr.protocol);
    
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 2: Read SIM Card Information (ICCID, IMSI) */
/* ========================================================================== */

void example_read_sim_info(void) {
    void *uart_handle = NULL; /* Pre-configured UART */
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    /* Initialize */
    xy_iso7816_init(&iso_handle, uart_handle);
    
    /* Reset card */
    xy_iso7816_atr_t atr;
    ret = xy_iso7816_reset(&iso_handle, &atr);
    if (ret != XY_ISO7816_OK) {
        printf("Card reset failed\n");
        return;
    }
    
    /* Get complete SIM information */
    xy_iso7816_sim_info_t sim_info;
    ret = xy_iso7816_get_sim_info(&iso_handle, &sim_info);
    if (ret != XY_ISO7816_OK) {
        printf("Failed to read SIM info\n");
        return;
    }
    
    /* Print card type */
    const char *card_type_str[] = {
        "Unknown", "SIM (2G)", "USIM (3G)", "ISIM", "Generic"
    };
    printf("Card Type: %s\n", card_type_str[sim_info.card_type]);
    
    /* Print ICCID */
    char iccid_str[21];
    xy_iso7816_bcd_to_ascii(sim_info.iccid, sim_info.iccid_len, 
                            iccid_str, sizeof(iccid_str));
    printf("ICCID: %s\n", iccid_str);
    
    /* Print IMSI */
    char imsi_str[16];
    xy_iso7816_bcd_to_ascii(&sim_info.imsi[1], sim_info.imsi[0],
                            imsi_str, sizeof(imsi_str));
    printf("IMSI: %s\n", imsi_str);
    
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 3: PIN Verification */
/* ========================================================================== */

void example_verify_pin(const char *pin) {
    void *uart_handle = NULL;
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    xy_iso7816_init(&iso_handle, uart_handle);
    
    xy_iso7816_atr_t atr;
    xy_iso7816_reset(&iso_handle, &atr);
    
    /* Verify PIN */
    xy_u8 remaining_tries = 0;
    ret = xy_iso7816_verify_pin(&iso_handle, pin, &remaining_tries);
    
    if (ret == XY_ISO7816_OK) {
        printf("PIN verification successful\n");
    } else {
        printf("PIN verification failed. Remaining tries: %d\n", remaining_tries);
    }
    
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 4: 2G Authentication (GET CHALLENGE) */
/* ========================================================================== */

void example_2g_authentication(void) {
    void *uart_handle = NULL;
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    xy_iso7816_init(&iso_handle, uart_handle);
    
    xy_iso7816_atr_t atr;
    xy_iso7816_reset(&iso_handle, &atr);
    
    /* Get random challenge from SIM */
    xy_u8 rand[16];
    ret = xy_iso7816_get_challenge(&iso_handle, rand);
    
    if (ret == XY_ISO7816_OK) {
        printf("Challenge received: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X", rand[i]);
        }
        printf("\n");
        
        /* In real application, you would:
         * 1. Send RAND to network
         * 2. Receive SRES and Kc from network
         * 3. Use for authentication and encryption
         */
    } else {
        printf("Failed to get challenge\n");
    }
    
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 5: 3G/4G Mutual Authentication */
/* ========================================================================== */

void example_3g_authentication(void) {
    void *uart_handle = NULL;
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    xy_iso7816_init(&iso_handle, uart_handle);
    
    xy_iso7816_atr_t atr;
    xy_iso7816_reset(&iso_handle, &atr);
    
    /* Example authentication vectors (from network) */
    xy_u8 rand[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    xy_u8 autn[16] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
    };
    
    /* Response buffers */
    xy_u8 res[8];     /* Authentication response */
    xy_u8 ck[16];     /* Cipher key */
    xy_u8 ik[16];     /* Integrity key */
    
    ret = xy_iso7816_authenticate(&iso_handle, rand, autn, res, ck, ik);
    
    if (ret == XY_ISO7816_OK) {
        printf("Authentication successful\n");
        
        printf("RES: ");
        for (int i = 0; i < 8; i++) {
            printf("%02X", res[i]);
        }
        printf("\n");
        
        printf("CK: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X", ck[i]);
        }
        printf("\n");
        
        printf("IK: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X", ik[i]);
        }
        printf("\n");
        
        /* In real application:
         * - Send RES to network for verification
         * - Use CK and IK for encryption/integrity protection
         */
    } else {
        printf("Authentication failed\n");
    }
    
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 6: Manual File Selection and Reading */
/* ========================================================================== */

void example_manual_file_access(void) {
    void *uart_handle = NULL;
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    xy_iso7816_init(&iso_handle, uart_handle);
    
    xy_iso7816_atr_t atr;
    xy_iso7816_reset(&iso_handle, &atr);
    
    /* Select Master File */
    ret = xy_iso7816_select_file(&iso_handle, XY_ISO7816_FID_MF);
    if (ret != XY_ISO7816_OK) {
        printf("Failed to select MF\n");
        goto cleanup;
    }
    
    /* Select Telecom DF */
    ret = xy_iso7816_select_file(&iso_handle, XY_ISO7816_FID_DF_TELECOM);
    if (ret != XY_ISO7816_OK) {
        printf("Failed to select DF_TELECOM\n");
        goto cleanup;
    }
    
    /* Select and read a specific file */
    /* Example: Read Administrative Data (EF_AD) */
    ret = xy_iso7816_select_file(&iso_handle, XY_ISO7816_FID_EF_AD);
    if (ret == XY_ISO7816_OK) {
        xy_u8 ad_data[4];
        ret = xy_iso7816_read_binary(&iso_handle, 0, ad_data, sizeof(ad_data));
        
        if (ret == XY_ISO7816_OK) {
            printf("Administrative Data: ");
            for (int i = 0; i < 4; i++) {
                printf("%02X ", ad_data[i]);
            }
            printf("\n");
        }
    }
    
cleanup:
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 7: Custom APDU Command */
/* ========================================================================== */

void example_custom_apdu(void) {
    void *uart_handle = NULL;
    xy_iso7816_handle_t iso_handle;
    
    xy_iso7816_init(&iso_handle, uart_handle);
    
    xy_iso7816_atr_t atr;
    xy_iso7816_reset(&iso_handle, &atr);
    
    /* Create custom APDU command */
    xy_iso7816_apdu_cmd_t cmd;
    xy_iso7816_apdu_resp_t resp;
    
    memset(&cmd, 0, sizeof(cmd));
    cmd.cla = XY_ISO7816_CLA_GSM;
    cmd.ins = 0xF2;  /* Custom instruction */
    cmd.p1 = 0x00;
    cmd.p2 = 0x00;
    cmd.lc = 0;
    cmd.le = 0;
    
    xy_iso7816_error_t ret = xy_iso7816_transceive(&iso_handle, &cmd, &resp);
    
    if (ret == XY_ISO7816_OK) {
        xy_u16 sw = xy_iso7816_get_sw(&resp);
        printf("Status Word: 0x%04X\n", sw);
        
        if (resp.length > 0) {
            printf("Response data (%d bytes): ", resp.length);
            for (xy_u16 i = 0; i < resp.length; i++) {
                printf("%02X ", resp.data[i]);
            }
            printf("\n");
        }
    }
    
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Example 8: Complete SIM Card Workflow */
/* ========================================================================== */

void example_complete_workflow(const char *pin) {
    void *uart_handle = NULL;
    xy_iso7816_handle_t iso_handle;
    xy_iso7816_error_t ret;
    
    printf("=== ISO7816 SIM Card Complete Workflow ===\n\n");
    
    /* Step 1: Initialize */
    ret = xy_iso7816_init(&iso_handle, uart_handle);
    if (ret != XY_ISO7816_OK) {
        printf("Initialization failed\n");
        return;
    }
    printf("✓ ISO7816 initialized\n");
    
    /* Step 2: Reset and get ATR */
    xy_iso7816_atr_t atr;
    ret = xy_iso7816_reset(&iso_handle, &atr);
    if (ret != XY_ISO7816_OK) {
        printf("Card reset failed\n");
        goto cleanup;
    }
    printf("✓ Card reset successful, ATR length: %d bytes\n", atr.length);
    
    /* Step 3: Detect card type */
    xy_iso7816_card_type_t card_type;
    ret = xy_iso7816_detect_card_type(&iso_handle, &card_type);
    if (ret == XY_ISO7816_OK) {
        const char *type_names[] = {"Unknown", "SIM", "USIM", "ISIM", "Generic"};
        printf("✓ Card type: %s\n", type_names[card_type]);
    }
    
    /* Step 4: Read ICCID */
    xy_u8 iccid[10];
    xy_u8 iccid_len;
    ret = xy_iso7816_read_iccid(&iso_handle, iccid, &iccid_len);
    if (ret == XY_ISO7816_OK) {
        char iccid_str[21];
        xy_iso7816_bcd_to_ascii(iccid, iccid_len, iccid_str, sizeof(iccid_str));
        printf("✓ ICCID: %s\n", iccid_str);
    }
    
    /* Step 5: Verify PIN if provided */
    if (pin != NULL) {
        xy_u8 remaining;
        ret = xy_iso7816_verify_pin(&iso_handle, pin, &remaining);
        if (ret == XY_ISO7816_OK) {
            printf("✓ PIN verified successfully\n");
        } else {
            printf("✗ PIN verification failed (remaining: %d)\n", remaining);
            goto cleanup;
        }
    }
    
    /* Step 6: Read IMSI */
    xy_u8 imsi[9];
    xy_u8 imsi_len;
    ret = xy_iso7816_read_imsi(&iso_handle, imsi, &imsi_len);
    if (ret == XY_ISO7816_OK) {
        char imsi_str[16];
        xy_iso7816_bcd_to_ascii(&imsi[1], imsi[0], imsi_str, sizeof(imsi_str));
        printf("✓ IMSI: %s\n", imsi_str);
    }
    
    /* Step 7: Get authentication challenge (for 2G) */
    xy_u8 challenge[16];
    ret = xy_iso7816_get_challenge(&iso_handle, challenge);
    if (ret == XY_ISO7816_OK) {
        printf("✓ Challenge obtained\n");
    }
    
    printf("\n=== Workflow completed ===\n");
    
cleanup:
    xy_iso7816_deinit(&iso_handle);
}

/* ========================================================================== */
/* Main function (for testing) */
/* ========================================================================== */

#ifdef ISO7816_EXAMPLE_MAIN
int main(void) {
    /* Uncomment the example you want to run */
    
    // example_init_and_atr();
    // example_read_sim_info();
    // example_verify_pin("1234");
    // example_2g_authentication();
    // example_3g_authentication();
    // example_manual_file_access();
    // example_custom_apdu();
    
    example_complete_workflow("1234");
    
    return 0;
}
#endif
