#include "sensor_mlx90393.h"
#include <string.h>
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static sensor_err_t mlx90393_init(sensor_device_t *s){SENSOR_LOG("Initializing MLX90393");return SENSOR_EOK;}
static sensor_err_t mlx90393_read(sensor_device_t *s, sensor_data_t *d){
    d->type=SENSOR_TYPE_ANGLE;d->value.val_float=0.0f;d->timestamp=SENSOR_GET_TICK();return SENSOR_EOK;
}
static const sensor_ops_t mlx90393_ops = {.init=mlx90393_init,.read=mlx90393_read};
sensor_device_t *mlx90393_create(const char *name, void *i2c_bus){
    sensor_device_t *s=(sensor_device_t*)SENSOR_MALLOC(sizeof(sensor_device_t));mlx90393_priv_t *p=(mlx90393_priv_t*)SENSOR_MALLOC(sizeof(mlx90393_priv_t));
    if(!s||!p){SENSOR_FREE(s);SENSOR_FREE(p);return NULL;}
    memset(s,0,sizeof(sensor_device_t));p->i2c_addr=MLX90393_ADDR;
    strncpy(s->info.name,name,SENSOR_NAME_MAX_LEN-1);s->info.vendor="Melexis";s->info.model="MLX90393";s->info.type=SENSOR_TYPE_ANGLE;
    s->ops=&mlx90393_ops;s->bus=i2c_bus;s->priv_data=p;s->status=SENSOR_STATUS_IDLE;return s;
}
