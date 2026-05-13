#include "sensor_dmp6100.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t dmp6100_init(sensor_device_t *sensor)
{SENSOR_LOG("Initializing DMP6100");hal_i2c_mem_write(sensor->bus,((dmp6100_priv_t*)sensor->priv_data)->i2c_addr,0x0C,(uint8_t[]){0x56},1);return SENSOR_EOK;}
static sensor_err_t dmp6100_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6]; dmp6100_priv_t *p=(dmp6100_priv_t*)sensor->priv_data;
    for(int i=0;i<6;i++) hal_i2c_mem_read(sensor->bus,p->i2c_addr,0x18+i,&buf[i],1);
    data->type=SENSOR_TYPE_ACCELEROMETER;data->value.val_3axis.x=((int16_t)((buf[1]<<8)|buf[0]))*16;
    data->value.val_3axis.y=((int16_t)((buf[3]<<8)|buf[2]))*16;data->value.val_3axis.z=((int16_t)((buf[5]<<8)|buf[4]))*16;
    data->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t dmp6100_ops = { .init=dmp6100_init, .read=dmp6100_read };
sensor_device_t *dmp6100_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));dmp6100_priv_t *p=(dmp6100_priv_t*)SENSOR_MALLOC(sizeof(dmp6100_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=addr?:0x12;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="国产";s->info.model="DMP6100";s->info.type=SENSOR_TYPE_ACCELEROMETER;
    s->ops=&dmp6100_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
