#ifndef ATC_CELLULAR_COMMON_H_
#define ATC_CELLULAR_COMMON_H_

// 定义一堆接口，具体实现根据不同的模块去实现

int8_t atc_cellular_rssi(const char *cmd);

/** return len of ate. */
int_t atc_celluar_version(char *version);

#endif