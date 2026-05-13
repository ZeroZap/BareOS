#include "sensor_pa122.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t pa122_init(sensor_device_t *s){SENSOR_LOG("Initializing PA122");return SENSOR_EOK;}
static sensor_err_t pa122_read(sensor_device_t *s, sensor_data_t *d){
    uint8_t st;pa122_priv_t *p=(pa122_priv_t*)s->priv_data;
    hal_i2c_mem_read(s->bus,p->i2c_addr,0x08,&st,1);
    d->type=SENSOR_TYPE_PROXIMITY;d->value.val_float=(st&0x80)?50.0f:200.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t pa122_ops = {.init=pa122_init,.read=pa122_read};
sensor_device_t *pa122_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));
    pa122_priv_t *p=(pa122_priv_t*)SENSOR_MALLOC(sizeof(pa122_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=PA122_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Perela";s->info.model="PA122";s->info.type=SENSOR_TYPE_PROXIMITY;
    s->ops=&pa122_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
