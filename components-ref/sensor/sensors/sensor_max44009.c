#include "sensor_max44009.h"
#include <string.h>
#include <math.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t max44009_init(sensor_device_t *s){SENSOR_LOG("Initializing MAX44009");return SENSOR_EOK;}
static sensor_err_t max44009_read(sensor_device_t *s, sensor_data_t *d){
    uint8_t buf[2];max44009_priv_t *p=(max44009_priv_t*)s->priv_data;
    hal_i2c_mem_read(s->bus,p->i2c_addr,0x03,&buf[0],1);hal_i2c_mem_read(s->bus,p->i2c_addr,0x04,&buf[1],1);
    int m=((buf[0]&0x0F)<<4)|(buf[1]&0x0F),e=(buf[0]&0xF0)>>4;
    d->type=SENSOR_TYPE_LIGHT;d->unit=SENSOR_UNIT_LUX;d->value.val_float=pow(2,e)*m*0.045f;
    d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t max44009_ops = {.init=max44009_init,.read=max44009_read};
sensor_device_t *max44009_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));
    max44009_priv_t *p=(max44009_priv_t*)SENSOR_MALLOC(sizeof(max44009_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=MAX44009_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Maxim";s->info.model="MAX44009";s->info.type=SENSOR_TYPE_LIGHT;
    s->ops=&max44009_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
