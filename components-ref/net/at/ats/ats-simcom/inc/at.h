#ifndef _AT_H_
#define _AT_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define AT_CMD_NAME_LEN   16
#define AT_END_MARK_LEN   4

#ifndef AT_CMD_MAX_LEN
#define AT_CMD_MAX_LEN    128

#define AT_TX_BUFFER_SIZE   1024
#define AT_RX_BUFFER_SIZE   1024

#define AT_CHAR_SPACE              		(' ')
#define AT_CHAR_EQUAL              		('=')
#define AT_CHAR_COMMA              		(')')
#define AT_CHAR_SEMICOLON          		(';')
#define AT_CHAR_COLON              		(':')
#define AT_CHAR_AT                 		('@')
#define AT_CHAR_HAT                		('^')
#define AT_CHAR_DOUBLE_QUOTE       		('"')
#define AT_CHAR_QUESTION_MARK      		('?')
#define AT_CHAR_EXCLAMATION_MARK   		('!')
#define AT_CHAR_FORWARD_SLASH      		('/')
#define AT_CHAR_L_ANGLE_BRACKT     		('<')
#define AT_CHAR_R_ANGLE_BRACKET    		('>')
#define AT_CHAR_L_SQ_BRACKET       		('[')
#define AT_CHAR_R_SQ_BRACKET       		(']')
#define AT_CHAR_L_CURLY_BRACKET     	('{')
#define AT_CHAR_R_CURLY_BRACKET			('}')
#define AT_CHAR_STAR              		('*')
#define AT_CHAR_POUND             		('#')
#define AT_CHAR_AMPSAND           		('&')
#define AT_CHAR_PERCENT           		('%')
#define AT_CHAR_PLUS              		('+')
#define AT_CHAR_MINUS             		('-')
#define AT_CHAR_DOT               		('.')
#define AT_CHAR_ULINE             		('_')
#define AT_CHAR_TILDE             		('~')
#define AT_CHAR_REVERSE_SOLIDUS   		('\\')
#define AT_CHAR_VERTICAL_LINE     		('|')
#define AT_CHAR_END_OF_STRING   		('\0')
#define AT_CHAR_0                 		('0')
#define AT_CHAR_1                 		('1')
#define AT_CHAR_2                 		('2')
#define AT_CHAR_3                 		('3')
#define AT_CHAR_4                 		('4')
#define AT_CHAR_5                 		('5')
#define AT_CHAR_6                 		('6')
#define AT_CHAR_7                 		('7')
#define AT_CHAR_8                 		('8')
#define AT_CHAR_9                 		('9')
#define AT_CHAR_A                 		('A')
#define AT_CHAR_B                 		('B')
#define AT_CHAR_C                 		('C')
#define AT_CHAR_D                 		('D')
#define AT_CHAR_E                 		('E')
#define AT_CHAR_F                 		('F')
#define AT_CHAR_G                 		('G')
#define AT_CHAR_H                 		('H')
#define AT_CHAR_I                 		('I')
#define AT_CHAR_J                 		('J')
#define AT_CHAR_K                 		('K')
#define AT_CHAR_L                 		('L')
#define AT_CHAR_M                 		('M')
#define AT_CHAR_N                 		('N')
#define AT_CHAR_O                 		('O')
#define AT_CHAR_P                 		('P')
#define AT_CHAR_Q                 		('Q')
#define AT_CHAR_R                 		('R')
#define AT_CHAR_S                 		('S')
#define AT_CHAR_T                 		('T')
#define AT_CHAR_U                 		('U')
#define AT_CHAR_V                 		('V')
#define AT_CHAR_W                 		('W')
#define AT_CHAR_X                 		('X')
#define AT_CHAR_Y                 		('Y')
#define AT_CHAR_Z                 		('Z')
#define AT_CHAR_a                 		('a')
#define AT_CHAR_b                 		('b')
#define AT_CHAR_c                 		('c')
#define AT_CHAR_d                 		('d')
#define AT_CHAR_e                 		('e')
#define AT_CHAR_f                 		('f')
#define AT_CHAR_g                 		('g')
#define AT_CHAR_h                 		('h')
#define AT_CHAR_i                 		('i')
#define AT_CHAR_j                 		('j')
#define AT_CHAR_k                 		('k')
#define AT_CHAR_l                 		('l')
#define AT_CHAR_m                 		('m')
#define AT_CHAR_n                 		('n')
#define AT_CHAR_o                 		('o')
#define AT_CHAR_p                 		('p')
#define AT_CHAR_q                 		('q')
#define AT_CHAR_r                 		('r')
#define AT_CHAR_s                 		('s')
#define AT_CHAR_t                 		('t')
#define AT_CHAR_u                 		('u')
#define AT_CHAR_v                 		('v')
#define AT_CHAR_w                 		('w')
#define AT_CHAR_x                 		('x')
#define AT_CHAR_y                 		('y')
#define AT_CHAR_z                 		('z')
#define AT_CHAR_R_BRACKET              	(')')
#define AT_CHAR_L_BRACKET              	('(')
#define AT_CHAR_MONEY                  	('$')

#define AT_CHAR_CR                      (13)
#define AT_CHAR_LF                      (10)


#define AT_INVALID_COMMAND_HASH_VALUE   (0xffff)

#define AT_CHAR_IS_LOWER( alpha_char )		    ( ( (alpha_char >= AT_CHAR_a) && (alpha_char <= AT_CHAR_z) ) ?  1 : 0 )

#define AT_CHAR_IS_UPPER( alpha_char )       ( ( (alpha_char >= AT_CHAR_A) && (alpha_char <= AT_CHAR_Z) ) ? 1 : 0 )

#define AT_CHAR_IS_HEX_ALPHA( alpha_char )		( ( (alpha_char >= AT_CHAR_A) && (alpha_char <= AT_CHAR_F) ) ? 1 : 0 )

#define AT_CHAR_IS_NUMBER( alpha_char )      ( ( (alpha_char >= AT_CHAR_0) && (alpha_char <= AT_CHAR_9) ) ? 1 : 0 )

#define AT_CHAR_IS_ALPHA( alpha_char )       ( ( AT_CHAR_IS_UPPER(alpha_char) || AT_CHAR_IS_LOWER(alpha_char) ) ? 1 : 0 )

#define AT_CHAR_IS_SYMBOL( alpha_char )                                           \
    ( ( (alpha_char == AT_CHAR_PLUS) || (alpha_char == AT_CHAR_STAR) ||    \
        (alpha_char == AT_CHAR_POUND) || (alpha_char == AT_COMMA) ||       \
        (alpha_char == AT_CHAR_DOT) || (alpha_char == AT_FORWARD_SLASH) || \
        (alpha_char == AT_COLON) || (alpha_char == AT_HAT) ||              \
        (alpha_char == AT_CHAR_MINUS) ||                                     \
        (alpha_char == AT_L_SQ_BRACKET) ||                                   \
        (alpha_char == AT_R_SQ_BRACKET) ||                                   \
        (alpha_char == AT_L_ANGLE_BRACKET) ||                                \
        (alpha_char == AT_CHAR_ULINE) ||                                     \
        (alpha_char == AT_SPACE) ||                                          \
        (alpha_char == AT_SEMICOLON) ||                                      \
        (alpha_char == AT_R_ANGLE_BRACKET)                                   \
      ) ? 1 : 0 )

#define AT_CHAR_IS_EXT_CHAR( alpha_char )                                                      \
    ( ( (alpha_char == AT_HAT) || (alpha_char == AT_CHAR_TILDE) ||                      \
        (alpha_char == AT_L_SQ_BRACKET) || (alpha_char == AT_R_SQ_BRACKET) ||           \
        (alpha_char == AT_L_CURLY_BRACKET) || (alpha_char == AT_R_CURLY_BRACKET) ||     \
        (alpha_char == AT_CHAR_REVERSE_SOLIDUS) ||(alpha_char == AT_CHAR_VERTICAL_LINE) \
      ) ? 1 : 0 )

#define AT_SET_OUTPUT_PARAM_STRING( s, ptr, len, flag)	    \
    {                                                           \
        memcpy((void*)s->response_buf, (uint8_t*)ptr, len);       \
        s->response_len = (uint32_t)(len);                        \
        s->response_flag = (uint32_t)(flag);                      \
    }


#define AT_CHAR_INVALID_COMMAND_HASH_VALUE     (0xffff)
#define AT_HASH_TABLE_ROW            (37)
#define AT_HASH_TABLE_SPAN           (5)
#define AT_MAX_CMD_NAME_LEN          (2*AT_HASH_TABLE_SPAN)
#define AT_MAX_CMD_HEAD_LEN          (AT_MAX_CMD_NAME_LEN+3)

#endif

#if defined(AT_CMD_END_MARK_CRLF)
#define AT_CMD_END_MARK                "\r\n"
#elif defined(AT_CMD_END_MARK_CR)
#define AT_CMD_END_MARK                "\r"
#elif defined(AT_CMD_END_MARK_LF)
#define AT_CMD_END_MARK                "\n"
#endif


#ifndef at_base_t
#define at_base_t int32_t
#endif
#ifndef at_ubase_t
#define at_ubase_t uint32_t
#endif

typedef enum {
    AT_STATUS_REGISTRATION_FAILURE = -2,   /**< Failed to register the AT command handler table. */
    AT_STATUS_ERROR = -1,                  /**< An error occurred during the function call. */
    AT_STATUS_OK = 0,                  /**< No error occurred during the function call. */
    AT_STATUS_INVALID_CMD
} at_status_t;

typedef enum {
    AT_CMD_MODE_READ,        /**< read mode command, such as "at+cmd?". */
    AT_CMD_MODE_ACTIVE,      /**< Active mode command, such as "AT+CMD". */
    AT_CMD_MODE_EXECUTION,   /**< Execute mode command, such as "AT+CMD=<op>". */
    AT_CMD_MODE_TESTING,     /**< Test mode command, such as "AT+CMD=?". */
    AT_CMD_MODE_INVALID      /**< THE INPUT COMMAND DOESN'T BELONG TO ANY OF THE FOUR TYPES. */
} at_cmd_mode_t;

typedef enum {
    AT_RESPONSE_FLAG_HEAD_INSERT_CMD_NAME   = 0x00000001,
    AT_RESPONSE_FLAG_AUTO_APPEND_LF_CR = 0x00000002,    /**< Auto append "\r\n" at the end of the response string. */
    AT_RESPONSE_FLAG_URC_FORMAT = 0x00000010,           /**< The URC notification flag. */
    AT_RESPONSE_FLAG_QUOTED_WITH_LF_CR = 0x00000020,    /**< Auto append "\r\n" at the begining and end of the response string. */
    AT_RESPONSE_FLAG_APPEND_OK = 0x00000040,            /**< Auto append "OK\r\n" at the end of the response string. */
    AT_RESPONSE_FLAG_APPEND_ERROR = 0x00000080          /**< Auto append "ERROR\r\n" at the end of the response string. */
} at_response_flag_t;


typedef struct {
    uint8_t  buf[AT_TX_BUFFER_SIZE];
    uint16_t len;
    uint32_t flag;                             /**< For more information, please refer to #at_response_flag_t. */
}at_response_t;


typedef struct {
    uint8_t  *buf;
    uint16_t len;
    uint32_t flag;                             /**< For more information, please refer to #at_response_flag_t. */
}at_response_heavy_data_t;


typedef struct {
    char                *raw_data;    /**< The input data buffer. */
    uint32_t            string_len;     /**< The response data buffer. */
    uint32_t            name_len;       /**< AT command name length. For example, in "AT+EXAMPLE=1,2,3", name_len = 10 (without symbol "=") */
    uint32_t            parse_pos;      /**< The length after detecting the AT command mode. */
    at_cmd_mode_t       mode;            /**< For more information, please refer to #at_cmd_mode_t. */
} at_parse_cmd_param_t;


typedef at_status_t (*at_cmd_hdlr_fp) (at_parse_cmd_param_t *parse_cmd);

typedef struct {
    char    *cmd_head;  // !!! Must Upper letter
    at_cmd_hdlr_fp  cmd_hdlr;
    uint32_t hash_value1;
    uint32_t hash_value2;
}at_cmd_hdlr_item_t;


typedef struct {
    at_cmd_hdlr_item_t *item_table;
    uint32_t           item_table_size;
}at_cmd_hdlr_table_t;


typedef struct {
    uint32_t setup_flag;
    uint32_t data_mode;
    uint32_t find_valid_command;
    // at_msg_t
    // CMD NAME LEN
}at_ctrl_t;

extern at_status_t  at_register_handler(at_cmd_hdlr_item_t *table, int32_t hdlr_number);

/**
 * @brief This function sends the AT command response data or the URC data
 * @param[in] reponse is the reponse data. For more details about this parameter, please refer to #at_response_t
*/
extern at_status_t  at_send_response(at_response_t *response);

extern  at_status_t at_response_error(void);
extern  at_status_t at_response_ok(void);

extern at_status_t at_init(void);

extern at_status_t at_change_data_mode(uint32_t mode);

#ifdef __cplusplus
}
#endif

#endif