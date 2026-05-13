#include "sensor_fsr.h"
#include <string.h>
extern uint16_t hal_adc_read(uint8_t pin);
static sensor_err_t fsr_init(sensor_device_t *s){SENSOR_LOG("Initializing FSR");return SENSOR_EOK;}
static sensor_err_t fsr_read(sensor_device_t *s, sensor_data_t *d){d->type=SENSOR_TYPE_PRESSURE;d->value.val_float=hal_adc_read(((fsr_priv_t*)s->priv_data)->adc_pin)/4.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;}
static const sensor_ops_t fsr_ops = {.init=fsr_init,.read=fsr_read};
sensor_device_t *fsr_create(const char *name, uint8_t adc_pin){sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));fsr_priv_t *p=(fsr_priv_t*)SENSOR_MALLOC(sizeof(fsr_priv_t));if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}memset(s,0,sizeof(sensor_device_t));p->adc_pin=adc_pin;strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Interlink";s->info.model="FSR";s->info.type=SENSOR_TYPE_PRESSURE;s->ops=&fsr_ops;s->bus=NULL;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;}
