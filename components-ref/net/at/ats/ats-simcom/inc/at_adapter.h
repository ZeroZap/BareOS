#ifndef AT_ADAPTER_H
#define AT_ADAPTER_H
#include "at.h"

#define at_log_d    uvr_log_d
#define at_log_w    uvr_log_w
#define at_log_i    uvr_log_i
#define at_log_e    uvr_log_e
#define at_log_f    uvr_log_f
#define at_sleep    sAPI_TaskSleep

typedef void* at_msgq_t;
typedef void* at_mutex_t;
typedef void* at_semaphore_t;



/*--- Function ---*/
/* related with OS platform */
#define at_malloc   malloc
#define at_free     free
#define at_hw_interrupt_disable sAPI_ContextLock
#define at_hw_interrupt_enable  sAPI_ContextUnlock


extern at_msgq_t at_msgq_create(char *name, uint32_t max_size, uint32_t max_number);
extern at_status_t at_msgq_delete(at_msgq_t msgq);
extern at_status_t at_msgq_receive(at_msgq_t msgq, void *data, int32_t timeout);
extern at_status_t at_msgq_send(at_msgq_t msgq, void *data);
extern uint32_t at_msgq_get_num(at_msgq_t msgq);

extern at_mutex_t at_mutex_create(void);
extern at_status_t at_mutex_delete(at_mutex_t mutex);
extern at_status_t at_mutex_lock(at_mutex_t mutex);
extern at_status_t at_mutex_unlock(at_mutex_t mutex);


extern at_semaphore_t at_semaphore_create(uint32_t init_count);
extern at_status_t at_semaphore_delete(at_semaphore_t semaphore);
extern at_status_t at_semaphore_take(at_semaphore_t semaphore);
extern at_status_t at_semaphore_give(at_semaphore_t semaphore);

#endif