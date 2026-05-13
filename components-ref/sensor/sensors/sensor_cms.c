#include "sensor_cms.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t cms_init(sensor_device_t *sensor){SENSOR_LOG("Initializing CMS");hal_i2c_mem_write(sensor->bus,((cms_priv_t*)sensor->priv_data)->i2c_addr,0x20,(uint8_t[]){0x57},1);return SENSOR_EOK;}
static sensor_err_t cms_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6]; cms_priv_t *p=(cms_priv_t*)sensor->priv_data;
    for(int i=0;i<6;i++) hal_i2c_mem_read(sensor->bus,p->i2c_addr,0x28+i,&buf[i],1);
    data->type=SENSOR_TYPE_ACCELEROMETER; data->unit=SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x=(int16_t)((buf[1]<<8)|buf[0]);data->value.val_3axis.y=(int16_t)((buf[3]<<8)|buf[2]);data->value.val_3axis.z=(int16_t)((buf[5]<<8)|buf[4]);
    data->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t cms_ops = { .init=cms_init, .read=cms_read };
sensor_device_t *cms_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t)); cms_priv_t *p=(cms_priv_t*)SENSOR_MALLOC(sizeof(cms_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t)); p->i2c_addr=addr?:0x18;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1); s->info.vendor="CRmicro"; s->info.model="CMS"; s->info.type=SENSOR_TYPE_ACCELEROMETER;
    s->ops=&cms_ops; s->bus=i2c_bus; s->priv_data=p; s->status=SENSOR_STATUS_IDLE; return s;
}
