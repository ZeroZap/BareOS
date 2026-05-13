#include "sensor_max30102.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t max30102_init(sensor_device_t *s){SENSOR_LOG("Initializing MAX30102");return SENSOR_EOK;}
static sensor_err_t max30102_read(sensor_device_t *s, sensor_data_t *d){
    d->type=SENSOR_TYPE_HEART_RATE;d->value.val_float=72.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t max30102_ops = {.init=max30102_init,.read=max30102_read};
sensor_device_t *max30102_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));
    max30102_priv_t *p=(max30102_priv_t*)SENSOR_MALLOC(sizeof(max30102_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=MAX30102_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Maxim";s->info.model="MAX30102";s->info.type=SENSOR_TYPE_HEART_RATE;
    s->ops=&max30102_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
