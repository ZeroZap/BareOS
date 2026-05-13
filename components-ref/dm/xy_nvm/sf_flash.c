#include "sf.h"

#define FLASH_PAGE_SIZE	512
#define DATA_SIZE           32  // DATA_SIZE 最小为 = 16, 512/16 = 32bit，用32bit可以表示
#define DATA_NUM            ((FLASH_PAGE_SIZE)/(DATA_SIZE)-1)
#define FIRST_DATA_ADDR     ((FLASH_PAGE_SIZE)-2*(DATA_SIZE))
#define TABLE_SIZE          (DATA_SIZE/8)
#define TABLE_ADDR          (FLASH_PAGE_SIZE-DATA_SIZE)
#define COMBINE_ADDR        1
#define OFFSET_SHIFT        5 // 根据DATA_SIZE的值来移位，必须为2^n次方


sf_uint8_t current_sector = 0;
sf_uint8_t current_offset = 0;
sf_uint8_t sector_need_erase = 0;
sf_uint8_t table[DATA_SIZE];

void flash_data_write(sf_uint16_t address, sf_uint8_t *data, sf_uint16_t length)
{
    if(current_sector == 0) {
        Data_Area_Mass_Write(address, data, length);
    } else {
        Data_Area_Mass_Write(address+FLASH_PAGE_SIZE, data, length);
    }
}

void flash_data_read(sf_uint16_t address, sf_uint8_t *data, sf_uint16_t length)
{
    if(current_sector == 0) {
        Data_Area_Mass_Read(address, data, length);
    } else {
        Data_Area_Mass_Read(address+FLASH_PAGE_SIZE, data, length);
    }
}

void flash_sector_erase(sf_uint8_t sector)
{
    Data_Area_Sector_Erase(sector);
}

sf_uint8_t get_data_address_offset(void)
{
    current_offset=0;
    sf_uint8_t i,j;
    for(j = 0 ; j < DATA_NUM; j++) {
        for(i=0; i<8; i++)
        {
            if( ( (table[j]<<i)&0x80 ) == 0){
                current_offset++;
            } else {
                return current_offset;
            }
        }
    }
    return current_offset;
}

sf_uint8_t set_data_address_offset(void)
{

}

void check_page(void)
{
    sf_uint8_t sector;
    flash_data_read(FLASH_PAGE_SIZE-1, &table, 1);
    if(sector == 0xAA) {
        current_offset =  0;
    } else if (sector != 0xff) {
        sector_need_erase = (sector_need_erase&0xf0) | 0xf;
    }

    flash_data_read((FLASH_PAGE_SIZE<<1)-1, &table, 1);
    if(sector == 0x55) {
        current_offset = 1;
    } else if (sector != 0xff) {
        sector_need_erase = (sector_need_erase&0x0f) | 0xf0;
    }
}

void data_init(sf_uint8_t *data)
{
    flash_sector_erase(0);
    flash_sector_erase(1);
    return -1;


}

void data_write(sf_uint8_t *data)
{
    get_data_address_offset();
    check_page();
    if( (sector_need_erase&0x0f) == 0xf) {
        flash_sector_erase(0);
    }
    if( (sector_need_erase&0xf) == 0xf0) {
        flash_sector_erase(1);
    }
    flash_data_write((current_offset<<OFFSET_SHIFT), data, DATA_SIZE);
}

void data_read(sf_uint8_t *data)
{
    flash_data_read((current_offset<<OFFSET_SHIFT), data, DATA_SIZE);
}



