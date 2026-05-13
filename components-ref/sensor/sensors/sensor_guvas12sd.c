#include "sensor_guvas12sd.h"
#include <string.h>
extern uint16_t hal_adc_read(uint8_t pin);
static sensor_err_t guvas12sd_init(sensor_device_t *sensor){SENSOR_LOG("Initializing GUVA-S12SD");return SENSOR_EOK;}
static sensor_err_t guvas12sd_read(sensor_device_t *sensor, sensor_data_t *data)
{
    guvas12sd_priv_t *p=(guvas12sd_priv_t*)sensor->priv_data;
    data->type=SENSOR_TYPE_UV_INDEX;data->value.val_float=hal_adc_read(p->adc_pin)*0.1f;
    data->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t guvas12sd_ops = { .init=guvas12sd_init, .read=guvas12sd_read };
sensor_device_t *guvas12sd_create(const char *name, uint8_t adc_pin)
{
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));guvas12sd_priv_t *p=(guvas12sd_priv_t*)SENSOR_MALLOC(sizeof(guvas12sd_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->adc_pin=adc_pin;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="国产";s->info.model="GUVA-S12SD";s->info.type=SENSOR_TYPE_UV_INDEX;
    s->ops=&guvas12sd_ops;s->bus=NULL;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
