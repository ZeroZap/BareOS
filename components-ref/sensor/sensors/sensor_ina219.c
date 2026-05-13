#include "sensor_ina219.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t ina219_init(sensor_device_t *s){SENSOR_LOG("Initializing INA219");return SENSOR_EOK;}
static sensor_err_t ina219_read(sensor_device_t *s, sensor_data_t *d){
    uint8_t buf[2];ina219_priv_t *p=(ina219_priv_t*)s->priv_data;
    hal_i2c_mem_read(s->bus,p->i2c_addr,0x01,buf,2);
    int16_t v=((int16_t)buf[0]<<8)|buf[1];
    d->type=SENSOR_TYPE_CURRENT;d->value.val_float=v*0.1f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t ina219_ops = {.init=ina219_init,.read=ina219_read};
sensor_device_t *ina219_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));ina219_priv_t *p=(ina219_priv_t*)SENSOR_MALLOC(sizeof(ina219_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=INA219_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="TI";s->info.model="INA219";s->info.type=SENSOR_TYPE_CURRENT;
    s->ops=&ina219_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
