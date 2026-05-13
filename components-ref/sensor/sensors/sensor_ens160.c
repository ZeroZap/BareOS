#include "sensor_ens160.h"
#include <string.h>
static sensor_err_t ens160_init(sensor_device_t *s){SENSOR_LOG("Initializing ENS160");return SENSOR_EOK;}
static sensor_err_t ens160_read(sensor_device_t *s, sensor_data_t *d){d->type=SENSOR_TYPE_GAS;d->value.val_float=100.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;}
static const sensor_ops_t ens160_ops = {.init=ens160_init,.read=ens160_read};
sensor_device_t *ens160_create(const char *name, void *i2c_bus){sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));ens160_priv_t *p=(ens160_priv_t*)SENSOR_MALLOC(sizeof(ens160_priv_t));if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}memset(s,0,sizeof(sensor_device_t));p->i2c_addr=ENS160_ADDR;strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Sciosense";s->info.model="ENS160";s->info.type=SENSOR_TYPE_GAS;s->ops=&ens160_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;}
