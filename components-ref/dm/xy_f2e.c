#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define FLASH_PAGE_SIZE   1024
#define FLASH_TOTAL_PAGES 16
#define EEPROM_SIZE \
    (FLASH_PAGE_SIZE * FLASH_TOTAL_PAGES / 2) // 使用一半空间作为有效存储
#define INVALID_DATA 0xFFFF

typedef struct {
    uint16_t address;
    uint16_t data;
} EepromEntry;

// 模拟Flash存储
uint8_t flash_memory[FLASH_TOTAL_PAGES][FLASH_PAGE_SIZE];
uint32_t current_write_index = 0;

// Flash操作函数（这些函数应该根据实际MCU进行修改）
void flash_write(uint32_t address, const void *data, uint32_t size)
{
    memcpy(&flash_memory[address / FLASH_PAGE_SIZE][address % FLASH_PAGE_SIZE],
           data, size);
}

void flash_read(uint32_t address, void *data, uint32_t size)
{
    memcpy(data,
           &flash_memory[address / FLASH_PAGE_SIZE][address % FLASH_PAGE_SIZE],
           size);
}

void flash_erase_page(uint32_t page_address)
{
    memset(flash_memory[page_address / FLASH_PAGE_SIZE], 0xFF, FLASH_PAGE_SIZE);
}

// 查找最新的有效数据
bool find_latest_data(uint16_t address, uint16_t *data)
{
    for (int32_t i = current_write_index - sizeof(EepromEntry); i >= 0;
         i -= sizeof(EepromEntry)) {
        EepromEntry entry;
        flash_read(i, &entry, sizeof(EepromEntry));
        if (entry.address == address && entry.data != INVALID_DATA) {
            *data = entry.data;
            return true;
        }
    }
    return false;
}

// 写入数据
bool eeprom_write(uint16_t address, uint16_t data)
{
    if (address >= EEPROM_SIZE) {
        return false; // 地址超出范围
    }

    if (current_write_index + sizeof(EepromEntry)
        > FLASH_TOTAL_PAGES * FLASH_PAGE_SIZE) {
        // 需要压缩数据
        uint8_t temp_buffer[EEPROM_SIZE];
        memset(temp_buffer, 0xFF, EEPROM_SIZE);

        for (uint16_t addr = 0; addr < EEPROM_SIZE; addr += 2) {
            uint16_t latest_data;
            if (find_latest_data(addr, &latest_data)) {
                *(uint16_t *)&temp_buffer[addr] = latest_data;
            }
        }

        // 擦除所有页
        for (uint32_t page = 0; page < FLASH_TOTAL_PAGES; page++) {
            flash_erase_page(page * FLASH_PAGE_SIZE);
        }

        // 写回压缩后的数据
        flash_write(0, temp_buffer, EEPROM_SIZE);
        current_write_index = EEPROM_SIZE;
    }

    EepromEntry entry = { address, data };
    flash_write(current_write_index, &entry, sizeof(EepromEntry));
    current_write_index += sizeof(EepromEntry);

    return true;
}


// 读取数据
bool eeprom_read(uint16_t address, uint16_t *data)
{
    if (address >= EEPROM_SIZE) {
        return false; // 地址超出范围
    }

    return find_latest_data(address, data);
}

// 初始化
void eeprom_init()
{
    // 查找当前写入位置
    current_write_index = 0;
    while (current_write_index < FLASH_TOTAL_PAGES * FLASH_PAGE_SIZE) {
        EepromEntry entry;
        flash_read(current_write_index, &entry, sizeof(EepromEntry));
        if (entry.address == INVALID_DATA && entry.data == INVALID_DATA) {
            break;
        }
        current_write_index += sizeof(EepromEntry);
    }
}

// 使用示例
int main()
{
    eeprom_init();

    // 写入数据
    eeprom_write(0, 100);
    eeprom_write(2, 200);

    // 读取数据
    uint16_t read_data;
    if (eeprom_read(0, &read_data)) {
        // 使用读取的值
        printf("Address 0: %d\n", read_data);
    }

    if (eeprom_read(2, &read_data)) {
        printf("Address 2: %d\n", read_data);
    }

    return 0;
}