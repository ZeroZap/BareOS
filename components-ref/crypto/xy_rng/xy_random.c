#include <stdint.h>
#include <time.h>
#include "xy_rng.h"

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

// 简单的线性同余生成器状态
static uint32_t rng_state  = 1;
static int rng_initialized = 0;

static void init_rng(void)
{
    if (rng_initialized)
        return;

    // 使用时间和地址作为种子
    uint32_t seed = (uint32_t)time(NULL);
    seed ^= (uint32_t)(uintptr_t)&seed;

#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(
            &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptGenRandom(hProv, sizeof(seed), (BYTE *)&seed)) {
            // 使用系统随机数
        }
        CryptReleaseContext(hProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, &seed, sizeof(seed)) == sizeof(seed)) {
            // 使用系统随机数
        }
        close(fd);
    }
#endif

    rng_state       = seed;
    rng_initialized = 1;
}

// 简单的线性同余生成器
static uint32_t lcg_rand(void)
{
    rng_state = rng_state * 1103515245 + 12345;
    return rng_state;
}

uint32_t xy_random_uint32(void)
{
    if (!rng_initialized) {
        init_rng();
    }
    return lcg_rand();
}

int xy_random_bytes(uint8_t *buffer, size_t len)
{
    if (!buffer)
        return XY_RNG_INVALID_PARAM;

    if (!rng_initialized) {
        init_rng();
    }

#ifdef _WIN32
    // 尝试使用Windows CryptoAPI
    HCRYPTPROV hProv;
    if (CryptAcquireContext(
            &hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        BOOL success = CryptGenRandom(hProv, (DWORD)len, buffer);
        CryptReleaseContext(hProv, 0);
        if (success) {
            return XY_RNG_SUCCESS;
        }
    }
#else
    // 尝试使用 /dev/urandom
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t result = read(fd, buffer, len);
        close(fd);
        if (result == (ssize_t)len) {
            return XY_RNG_SUCCESS;
        }
    }
#endif

    // 降级到伪随机数生成器
    for (size_t i = 0; i < len; i += 4) {
        uint32_t rand_val = lcg_rand();
        size_t copy_len   = (len - i < 4) ? (len - i) : 4;
        for (size_t j = 0; j < copy_len; j++) {
            buffer[i + j] = (rand_val >> (j * 8)) & 0xFF;
        }
    }

    return XY_RNG_SUCCESS;
}