#include "sensor_mq135.h"
#include <string.h>
extern uint16_t hal_adc_read(uint8_t pin);
static sensor_err_t mq135_init(sensor_device_t *s){SENSOR_LOG("Initializing MQ135");return SENSOR_EOK;}
static sensor_err_t mq135_read(sensor_device_t *s, sensor_data_t *d){d->type=SENSOR_TYPE_GAS;d->value.val_float=hal_adc_read(((mq135_priv_t*)s->priv_data)->adc_pin)/8.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;}
static const sensor_ops_t mq135_ops = {.init=mq135_init,.read=mq135_read};
sensor_device_t *mq135_create(const char *name, uint8_t adc_pin){sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));mq135_priv_t *p=(mq135_priv_t*)SENSOR_MALLOC(sizeof(mq135_priv_t));if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}memset(s,0,sizeof(sensor_device_t));p->adc_pin=adc_pin;strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="国产";s->info.model="MQ-135";s->info.type=SENSOR_TYPE_GAS;s->ops=&mq135_ops;s->bus=NULL;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;}
