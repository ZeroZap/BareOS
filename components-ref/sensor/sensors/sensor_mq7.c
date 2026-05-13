#include "sensor_mq7.h"
#include <string.h>
extern uint16_t hal_adc_read(uint8_t pin);
static sensor_err_t mq7_init(sensor_device_t *s){SENSOR_LOG("Initializing MQ7");return SENSOR_EOK;}
static sensor_err_t mq7_read(sensor_device_t *s, sensor_data_t *d){d->type=SENSOR_TYPE_GAS;d->value.val_float=hal_adc_read(((mq7_priv_t*)s->priv_data)->adc_pin)/10.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;}
static const sensor_ops_t mq7_ops = {.init=mq7_init,.read=mq7_read};
sensor_device_t *mq7_create(const char *name, uint8_t adc_pin){sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));mq7_priv_t *p=(mq7_priv_t*)SENSOR_MALLOC(sizeof(mq7_priv_t));if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}memset(s,0,sizeof(sensor_device_t));p->adc_pin=adc_pin;strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="国产";s->info.model="MQ-7";s->info.type=SENSOR_TYPE_GAS;s->ops=&mq7_ops;s->bus=NULL;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;}
