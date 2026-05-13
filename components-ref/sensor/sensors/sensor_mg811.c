#include "sensor_mg811.h"
#include <string.h>
extern uint16_t hal_adc_read(uint8_t pin);
static sensor_err_t mg811_init(sensor_device_t *s){SENSOR_LOG("Initializing MG811");return SENSOR_EOK;}
static sensor_err_t mg811_read(sensor_device_t *s, sensor_data_t *d){
    uint16_t adc=hal_adc_read(((mg811_priv_t*)s->priv_data)->adc_pin);
    d->type=SENSOR_TYPE_GAS;d->value.val_float=((float)adc/4096.0f)*10000.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t mg811_ops = {.init=mg811_init,.read=mg811_read};
sensor_device_t *mg811_create(const char *name, uint8_t adc_pin){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));
    mg811_priv_t *p=(mg811_priv_t*)SENSOR_MALLOC(sizeof(mg811_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->adc_pin=adc_pin;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="国产";s->info.model="MG811";s->info.type=SENSOR_TYPE_GAS;
    s->ops=&mg811_ops;s->bus=NULL;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
