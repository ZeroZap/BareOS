# FatFS 文件系统集成

**状态**: ⏳ 待集成  
**优先级**: 🔴 高

---

## 📦 库信息

- **名称**: FatFS
- **版本**: R0.15
- **许可证**: BSD-2-Clause
- **官网**: http://elm-chan.org/fsw/ff/00index_e.html
- **作者**: ChaN

---

## 🔧 集成步骤

### 1. 下载 FatFS

```bash
cd components/third_party/filesystem
wget http://elm-chan.org/fsw/ff/arch/ff15.zip
unzip ff15.zip
mv ff fatfs
```

### 2. 配置 CMake

```cmake
# components/third_party/filesystem/CMakeLists.txt

# FatFS 配置
add_library(fatfs STATIC
    fatfs/source/ff.c
    fatfs/source/ffsystem.c
    fatfs/source/ffunicode.c
)

target_include_directories(fatfs PUBLIC
    fatfs/source
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

# FatFS 移植层
add_library(xy_fatfs_port STATIC
    port/xy_fatfs_disk.c
    port/xy_fatfs_io.c
)

target_include_directories(xy_fatfs_port PUBLIC
    fatfs/source
    ${CMAKE_CURRENT_SOURCE_DIR}/port
)

target_link_libraries(xy_fatfs_port PUBLIC fatfs)
```

### 3. 创建磁盘 I/O 层

```c
/* components/third_party/filesystem/port/xy_fatfs_disk.c */

#include "ff.h"
#include "diskio.h"
#include "xy_sd.h"

static DSTATUS xy_disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    
    /* 初始化 SD 卡 */
    if (xy_sd_init() != XY_SD_OK) {
        return STA_NOINIT;
    }
    
    return 0;
}

static DSTATUS xy_disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return 0;
}

static DRESULT xy_disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) return RES_PARERR;
    
    /* 从 SD 卡读取数据 */
    if (xy_sd_read_blocks(sector, buff, count) != XY_SD_OK) {
        return RES_ERROR;
    }
    
    return RES_OK;
}

static DRESULT xy_disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) return RES_PARERR;
    
    /* 写入 SD 卡 */
    if (xy_sd_write_blocks(sector, buff, count) != XY_SD_OK) {
        return RES_ERROR;
    }
    
    return RES_OK;
}

static DRESULT xy_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) return RES_PARERR;
    
    switch (cmd) {
        case CTRL_SYNC:
            xy_sd_sync();
            return RES_OK;
            
        case GET_SECTOR_COUNT:
            *(LBA_t*)buff = xy_sd_get_sector_count();
            return RES_OK;
            
        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;
            
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1;
            return RES_OK;
            
        default:
            return RES_PARERR;
    }
}

/* FatFS 磁盘驱动接口 */
const DREGION Drv[1] = {
    {
        .disk_initialize = xy_disk_initialize,
        .disk_status = xy_disk_status,
        .disk_read = xy_disk_read,
        .disk_write = xy_disk_write,
        .disk_ioctl = xy_disk_ioctl
    }
};
```

### 4. 系统集成层

```c
/* components/third_party/filesystem/port/xy_fatfs_io.c */

#include "ff.h"
#include "xy_fs.h"
#include <string.h>

/* FatFS 文件系统对象 */
static FATFS g_fatfs;

/* 初始化 FatFS */
int xy_fatfs_init(void)
{
    FRESULT res = f_mount(&g_fatfs, "", 1);
    if (res != FR_OK) {
        xy_log_e("FatFS mount failed: %d\n", res);
        return -1;
    }
    
    xy_log_i("FatFS initialized\n");
    return 0;
}

/* 打开文件 */
xy_fs_file_t* xy_fatfs_open(const char *path, int flags)
{
    FIL *file = malloc(sizeof(FIL));
    if (!file) return NULL;
    
    BYTE mode = 0;
    if (flags & XY_FS_MODE_READ) mode |= FA_READ;
    if (flags & XY_FS_MODE_WRITE) mode |= FA_WRITE;
    if (flags & XY_FS_MODE_CREATE) mode |= FA_OPEN_ALWAYS;
    
    FRESULT res = f_open(file, path, mode);
    if (res != FR_OK) {
        free(file);
        return NULL;
    }
    
    return (xy_fs_file_t*)file;
}

/* 读取文件 */
int xy_fatfs_read(xy_fs_file_t *file, void *buf, size_t len)
{
    FIL *fil = (FIL*)file;
    UINT br;
    
    FRESULT res = f_read(fil, buf, len, &br);
    if (res != FR_OK) {
        return -1;
    }
    
    return br;
}

/* 写入文件 */
int xy_fatfs_write(xy_fs_file_t *file, const void *buf, size_t len)
{
    FIL *fil = (FIL*)file;
    UINT bw;
    
    FRESULT res = f_write(fil, buf, len, &bw);
    if (res != FR_OK) {
        return -1;
    }
    
    return bw;
}

/* 关闭文件 */
int xy_fatfs_close(xy_fs_file_t *file)
{
    FIL *fil = (FIL*)file;
    f_close(fil);
    free(file);
    return 0;
}

/* FatFS 文件系统操作集 */
const xy_fs_ops_t xy_fatfs_ops = {
    .open = xy_fatfs_open,
    .read = xy_fatfs_read,
    .write = xy_fatfs_write,
    .close = xy_fatfs_close,
};
```

### 5. 使用示例

```c
#include "xy_fatfs.h"

int main(void)
{
    /* 初始化 FatFS */
    xy_fatfs_init();
    
    /* 打开文件 */
    xy_fs_file_t *file = xy_fatfs_open("test.txt", XY_FS_MODE_READ);
    if (file) {
        char buf[256];
        int len = xy_fatfs_read(file, buf, sizeof(buf));
        xy_log_i("Read %d bytes: %s\n", len, buf);
        xy_fatfs_close(file);
    }
    
    /* 创建文件 */
    file = xy_fatfs_open("output.txt", XY_FS_MODE_WRITE | XY_FS_MODE_CREATE);
    if (file) {
        xy_fatfs_write(file, "Hello FatFS!", 12);
        xy_fatfs_close(file);
    }
}
```

---

## 📊 集成检查清单

- [ ] 下载 FatFS
- [ ] 配置 CMake
- [ ] 创建磁盘 I/O 层
- [ ] 实现系统集成层
- [ ] 测试文件读写
- [ ] 测试目录操作
- [ ] 添加文档

---

## 🎯 集成时间表

| 阶段 | 任务 | 工时 |
|------|------|------|
| **阶段 1** | 下载 + 配置 | 1h |
| **阶段 2** | 磁盘 I/O 层 | 3h |
| **阶段 3** | 系统集成层 | 3h |
| **阶段 4** | 测试 + 文档 | 1h |
| **总计** | - | **8h** |

---

## 📚 相关文档

- [FatFS 官方文档](http://elm-chan.org/fsw/ff/00index_e.html)
- [FatFS API 参考](http://elm-chan.org/fsw/ff/en/fun.html)
- [FatFS 移植指南](http://elm-chan.org/fsw/ff/en/res.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0 (XY) + BSD-2 (FatFS)
