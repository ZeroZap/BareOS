#include "sensor_vcnl4040.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t vcnl4040_init(sensor_device_t *s){SENSOR_LOG("Initializing VCNL4040");return SENSOR_EOK;}
static sensor_err_t vcnl4040_read(sensor_device_t *s, sensor_data_t *d){
    uint8_t buf[2];vcnl4040_priv_t *p=(vcnl4040_priv_t*)s->priv_data;
    hal_i2c_mem_read(s->bus,p->i2c_addr,0x08,buf,2);
    d->type=SENSOR_TYPE_PROXIMITY;d->value.val_float=(float)(((uint16_t)buf[1]<<8)|buf[0]);
    d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t vcnl4040_ops = {.init=vcnl4040_init,.read=vcnl4040_read};
sensor_device_t *vcnl4040_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));
    vcnl4040_priv_t *p=(vcnl4040_priv_t*)SENSOR_MALLOC(sizeof(vcnl4040_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=VCNL4040_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Vishay";s->info.model="VCNL4040";s->info.type=SENSOR_TYPE_PROXIMITY;
    s->ops=&vcnl4040_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
