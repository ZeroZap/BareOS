#ifndef _XY_F2E_H_
#define _XY_F2E_H_
typedef size_t (*f2e_wirte)(size_t addr, xy_uint8_t *pdata, uint16_t len);
typedef size_t (*f2e_read)(size_t addr, xy_uint8_t *pdata, uint16_t len);
typedef size_t (*f2e_erase)(size_t addr, xy_uint8_t *pdata, uint16_t len);
struct f2e {
    size_t start_addr;
    size_t end_addr;
    uint16_t page_len;
    uint16_t sector_len;
    uint16_t total_len;
} f2e_t;
#endif