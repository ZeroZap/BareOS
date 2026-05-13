#ifndef _XY_SYS_H_
#define _XY_SYS_H_

int xy_sys_reset(int reset_by);

int xy_sys_reboot_reason(void *data);

int xy_sys_get_chip_id(void *data);

int xy_sys_get_sw_ver(void *data);

int xy_sys_get_hw_ver(void *data);

int xy_sys_get_mac_addr(void *data);


#endif
