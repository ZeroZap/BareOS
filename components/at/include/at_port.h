/**
 * @Brief: The AT component drives the interface implementation
 * @Author: roger.luo
 * @Date: 2021-04-04
 * @Last Modified by: roger.luo
 * @Last Modified time: 2021-11-27
 */
#ifndef __AT_PORT_H__
#define __AT_PORT_H__

/**
 *@brief Default correct response identifier.
 */
#define AT_DEF_RESP_OK    "OK"     
/**
 *@brief Default error response identifier.
 */
#define AT_DEF_RESP_ERR   "ERROR"

/**
 *@brief Default command timeout (ms)
 */
#define AT_DEF_TIMEOUT    500  

/**
 *@brief Number of retries when a command timeout/error occurs.
 */
#define AT_DEF_RETRY      2   

/**
 *@brief Default URC frame receive timeout (ms).
 */
#define AT_URC_TIMEOUT    500

/**
 *@brief Maximum AT command send data length (only for variable parameter commands).
 */
#ifndef AT_MAX_CMD_LEN
#define AT_MAX_CMD_LEN    256
#endif

/**
 *@brief Maximum number of work in queue (limit memory usage).
 */
#ifndef AT_LIST_WORK_COUNT
#define AT_LIST_WORK_COUNT 8
#endif

/**
 *@brief Use a fixed block pool for command work items instead of malloc/free.
 */
#ifndef AT_WORK_STATIC_POOL_EN
#define AT_WORK_STATIC_POOL_EN 1u
#endif

/**
 *@brief Number of global static work item blocks.
 *       Increase this when multiple AT objects can queue work concurrently.
 */
#ifndef AT_WORK_POOL_COUNT
#define AT_WORK_POOL_COUNT AT_LIST_WORK_COUNT
#endif

/**
 *@brief Maximum copied payload bytes stored inside one static work item.
 *       at_exec_cmd() needs AT_MAX_CMD_LEN; at_send_data() must not exceed this.
 */
#ifndef AT_WORK_ITEM_DATA_SIZE
#define AT_WORK_ITEM_DATA_SIZE AT_MAX_CMD_LEN
#endif
 
/**
 *@brief Enable URC watcher.
 */
#define AT_URC_WARCH_EN     1

/**
 *@brief A list of specified URC end marks (fill in as needed, the fewer the better).
 */
#define AT_URC_END_MARKS  ":,\n"
/**
 *@brief Enable memory watcher.
 */
#ifndef AT_MEM_WATCH_EN
#define AT_MEM_WATCH_EN     1u
#endif
  
/**
 *@brief Maximum memory usage limit (Valid when AT_MEM_WATCH_EN is enabled)
 */
#ifndef AT_MEM_LIMIT_SIZE
#define AT_MEM_LIMIT_SIZE   (3 * 1024)
#endif

/**
 *@brief Enable AT work context interfaces.
 */
#define AT_WORK_CONTEXT_EN  1u

/**
 * @brief Supports raw data transparent transmission
 */
#define AT_RAW_TRANSPARENT_EN  1u

void *at_malloc(unsigned int nbytes);

void  at_free(void *ptr);

unsigned int at_get_ms(void);

#endif
