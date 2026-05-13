/**
 * @file xy_version.h
 * @brief XinYi Framework Version Management
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_VERSION_H
#define XY_VERSION_H

/**
 * @page version 版本管理
 * 
 * 版本号格式：MAJOR.MINOR.PATCH
 * - MAJOR: 不兼容的 API 修改
 * - MINOR: 向后兼容的功能新增
 * - PATCH: 向后兼容的问题修复
 */

/* ==================== 版本号 ==================== */

#define XY_VERSION_MAJOR       1
#define XY_VERSION_MINOR       0
#define XY_VERSION_PATCH       0

/* ==================== 版本字符串 ==================== */

#define XY_VERSION_STRING      "1.0.0"

/* ==================== 版本数值 (便于比较) ==================== */

#define XY_VERSION_NUMBER      ((XY_VERSION_MAJOR << 16) | \
                                (XY_VERSION_MINOR << 8)  | \
                                (XY_VERSION_PATCH))

/* ==================== 版本检查宏 ==================== */

/**
 * @brief 检查版本是否至少为指定版本
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁号
 * @return 1=满足，0=不满足
 */
#define XY_VERSION_AT_LEAST(major, minor, patch) \
    (XY_VERSION_NUMBER >= (((major) << 16) | ((minor) << 8) | (patch)))

/**
 * @brief 检查版本是否完全匹配
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁号
 * @return 1=匹配，0=不匹配
 */
#define XY_VERSION_IS(major, minor, patch) \
    (XY_VERSION_NUMBER == (((major) << 16) | ((minor) << 8) | (patch)))

/**
 * @brief 检查版本是否兼容
 * @param major 主版本号
 * @return 1=兼容，0=不兼容
 */
#define XY_VERSION_IS_COMPATIBLE(major) \
    (XY_VERSION_MAJOR == (major))

/* ==================== 版本信息 API ==================== */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取版本号
 * @return 版本号字符串
 */
const char* xy_version_get_string(void);

/**
 * @brief 获取主版本号
 * @return 主版本号
 */
int xy_version_get_major(void);

/**
 * @brief 获取次版本号
 * @return 次版本号
 */
int xy_version_get_minor(void);

/**
 * @brief 获取补丁号
 * @return 补丁号
 */
int xy_version_get_patch(void);

/**
 * @brief 检查运行时版本
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁号
 * @return 1=兼容，0=不兼容
 */
int xy_version_check_compatibility(int major, int minor, int patch);

#ifdef __cplusplus
}
#endif

#endif /* XY_VERSION_H */
