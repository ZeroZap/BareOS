#include "at.h"
#include "at_adapter.h"
#include "uvr.h"

void *at_mem_alloc(size_t size)
{

    return malloc(size);
}

void at_mem_free(void *buf)
{
    if (NULL != buf)
        return free(buf);
}

at_msgq_t at_msgq_create(char *name, uint32_t max_size, uint32_t max_num)
{
    sMsgQRef msgq = NULL;
    if (SC_SUCCESS == sAPI_MsgQCreate(&msgq, name, max_size, max_num, SC_FIFO))
    {
        return (at_msgq_t)msgq;
    }
    else
    {
        return NULL;
    }
}

at_status_t at_msgq_delete(at_msgq_t msgq)
{
    if (NULL != msgq)
    {
        return sAPI_MsgQDelete((sMsgQRef)msgq);
    }
}

at_status_t at_msgq_receive(at_msgq_t msgq, void *data, int32_t timeout)
{
    // return sAPI_MsgQRecv((sMsgQRef) msgq, (SIM_MSG_T *)data, timeout);
    return sAPI_MsgQRecvEx((sMsgQRef)msgq, (SIM_MSG_T *)data, sizeof(SIM_MSG_T), timeout);
}

at_status_t at_msgq_send(at_msgq_t msgq, void *data)
{
    // return sAPI_MsgQSend((sMsgQRef) msgq, (SIM_MSG_T *)data);
    return sAPI_MsgQSendEx((sMsgQRef)msgq, sizeof(SIM_MSG_T), (SIM_MSG_T *)data, SC_NO_SUSPEND);
}

uint32_t at_msgq_get_num(at_msgq_t msgq)
{
    uint32_t num;
    sAPI_MsgQPoll((sMsgQRef)msgq, (uint32_t *)&num);
    return num;
}

at_mutex_t at_mutex_create(void)
{
    sMutexRef mutex;
    sAPI_MutexCreate(&mutex, SC_FIFO);
    return mutex;
}

at_status_t at_mutex_delete(at_mutex_t mutex)
{
    return sAPI_MutexDelete((sMutexRef)mutex);
}

at_status_t at_mutex_lock(at_mutex_t mutex)
{
    return sAPI_MutexLock((sMutexRef)mutex, SC_SUSPEND);
}

at_status_t at_mutex_unlock(at_mutex_t mutex)
{
    return sAPI_MutexUnLock((sMutexRef)mutex);
}

at_semaphore_t at_semaphore_create(uint32_t init_count)
{
    sSemaRef semaphore;
    sAPI_SemaphoreCreate(&semaphore, init_count, SC_FIFO);
    return semaphore;
}

at_status_t at_semaphore_delete(at_semaphore_t semaphore)
{
    return sAPI_SemaphoreDelete((sSemaRef)semaphore);
}

at_status_t at_semaphore_take(at_semaphore_t semaphore)
{
    return sAPI_SemaphoreAcquire((sSemaRef)semaphore, SC_SUSPEND);
}

at_status_t at_semaphore_give(at_semaphore_t semaphore)
{
    return sAPI_SemaphoreRelease((sSemaRef)semaphore);
}

#define AT_HANDLER "at_handler"