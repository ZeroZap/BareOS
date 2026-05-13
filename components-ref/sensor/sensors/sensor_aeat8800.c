#include "sensor_aeat8800.h"
#include <string.h>
static sensor_err_t aeat8800_init(sensor_device_t *s){SENSOR_LOG("Initializing AEAT-8800");return SENSOR_EOK;}
static sensor_err_t aeat8800_read(sensor_device_t *s, sensor_data_t *d){
    d->type=SENSOR_TYPE_ANGLE;d->value.val_float=0.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t aeat8800_ops = {.init=aeat8800_init,.read=aeat8800_read};
sensor_device_t *aeat8800_create(const char *name, void *spi_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));aeat8800_priv_t *p=(aeat8800_priv_t*)SENSOR_MALLOC(sizeof(aeat8800_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Bourns";s->info.model="AEAT-8800";s->info.type=SENSOR_TYPE_ANGLE;
    s->ops=&aeat8800_ops;s->bus=spi_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
