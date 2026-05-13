#include "sensor_hs_ads1100.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t hs_ads1100_init(sensor_device_t *sensor)
{
    SENSOR_LOG("Initializing HS-ADS1100");
    hal_i2c_mem_write(sensor->bus, ((hs_ads1100_priv_t*)sensor->priv_data)->i2c_addr, 0x20, (uint8_t[]){0x57}, 1);
    return SENSOR_EOK;
}
static sensor_err_t hs_ads1100_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6]; hs_ads1100_priv_t *p = (hs_ads1100_priv_t*)sensor->priv_data;
    for(int i=0;i<6;i++) hal_i2c_mem_read(sensor->bus,p->i2c_addr,0x28+i,&buf[i],1);
    data->type=SENSOR_TYPE_ACCELEROMETER; data->unit=SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x=(buf[1]<<8)|buf[0]; data->value.val_3axis.y=(buf[3]<<8)|buf[2]; data->value.val_3axis.z=(buf[5]<<8)|buf[4];
    data->timestamp=SENSOR_GET_TICK(); return SENSOR_EOK;
}
static const sensor_ops_t hs_ads1100_ops = { .init=hs_ads1100_init, .read=hs_ads1100_read };
sensor_device_t *hs_ads1100_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));
    hs_ads1100_priv_t *p=(hs_ads1100_priv_t*)SENSOR_MALLOC(sizeof(hs_ads1100_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t)); p->i2c_addr=addr?:0x18;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1); s->info.vendor="Hangshun"; s->info.model="HS-ADS1100"; s->info.type=SENSOR_TYPE_ACCELEROMETER;
    s->ops=&hs_ads1100_ops; s->bus=i2c_bus; s->priv_data=p; s->status=SENSOR_STATUS_IDLE; return s;
}
