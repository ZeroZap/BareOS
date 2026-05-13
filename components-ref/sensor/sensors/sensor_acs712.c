#include "sensor_acs712.h"
#include <string.h>
extern uint16_t hal_adc_read(uint8_t pin);
static sensor_err_t acs712_init(sensor_device_t *s){SENSOR_LOG("Initializing ACS712");return SENSOR_EOK;}
static sensor_err_t acs712_read(sensor_device_t *s, sensor_data_t *d){
    float v=(hal_adc_read(((acs712_priv_t*)s->priv_data)->adc_pin)/4096.0f*3.3f-1.65f)/0.066f;
    d->type=SENSOR_TYPE_CURRENT;d->value.val_float=v;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t acs712_ops = {.init=acs712_init,.read=acs712_read};
sensor_device_t *acs712_create(const char *name, uint8_t adc_pin){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));acs712_priv_t *p=(acs712_priv_t*)SENSOR_MALLOC(sizeof(acs712_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->adc_pin=adc_pin;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Allegro";s->info.model="ACS712";s->info.type=SENSOR_TYPE_CURRENT;
    s->ops=&acs712_ops;s->bus=NULL;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
