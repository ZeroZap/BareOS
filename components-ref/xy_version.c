/**
 * @file xy_version.c
 * @brief XinYi Framework Version Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_version.h"
#include <stdio.h>

const char* xy_version_get_string(void)
{
    return XY_VERSION_STRING;
}

int xy_version_get_major(void)
{
    return XY_VERSION_MAJOR;
}

int xy_version_get_minor(void)
{
    return XY_VERSION_MINOR;
}

int xy_version_get_patch(void)
{
    return XY_VERSION_PATCH;
}

int xy_version_check_compatibility(int major, int minor, int patch)
{
    /* 主版本号必须匹配 */
    if (major != XY_VERSION_MAJOR) {
        return 0;
    }
    
    /* 次版本号或补丁号更高则兼容 */
    if (minor > XY_VERSION_MINOR) {
        return 1;
    }
    if (minor == XY_VERSION_MINOR && patch >= XY_VERSION_PATCH) {
        return 1;
    }
    
    return 0;
}
