#include "sensor_as5048.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t as5048_init(sensor_device_t *s){SENSOR_LOG("Initializing AS5048");return SENSOR_EOK;}
static sensor_err_t as5048_read(sensor_device_t *s, sensor_data_t *d){
    uint8_t buf[2];as5048_priv_t *p=(as5048_priv_t*)s->priv_data;
    hal_i2c_mem_read(s->bus,p->i2c_addr,0xFE,buf,2);
    d->type=SENSOR_TYPE_ANGLE;d->value.val_float=((uint16_t)buf[1]<<8|buf[0])/16384.0f*360.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t as5048_ops = {.init=as5048_init,.read=as5048_read};
sensor_device_t *as5048_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));as5048_priv_t *p=(as5048_priv_t*)SENSOR_MALLOC(sizeof(as5048_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=AS5048_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="AMS";s->info.model="AS5048";s->info.type=SENSOR_TYPE_ANGLE;
    s->ops=&as5048_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
