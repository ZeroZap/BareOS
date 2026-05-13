# XY Tiny Crypto

一个轻量级的 C 语言加密算法库，提供常用的加密、哈希和编码功能。

> ⚠️ **注意**: 本库主要用于学习和轻量级应用，不建议在高安全要求的生产环境中使用。

## 🆕 新特性 - 平台选择支持

现在支持三种实现方式:

1. **纯 C 软件实现** - 默认,完全可移植
2. **HAL 硬件加速** - 使用自定义硬件抽象层
3. **平台 SDK** - 直接使用芯片厂商库 (STM32/ESP32/Nordic)

**详细文档**: 请查看 [PLATFORM_GUIDE.md](PLATFORM_GUIDE.md)

### 快速配置

编辑 `include/xy_crypto_config.h`:

```c
// 选择平台
#define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_SOFTWARE  // 软件实现
// #define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_HAL    // HAL 加速
// #define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_SDK    // SDK 实现

// 软件除法支持 (与 xy_clib 的 XY_USE_SOFT_DIV 兼容)
#define XY_CRYPTO_USE_SOFT_DIV 0  // 0=硬件除法, 1=软件除法

// 选择性启用算法 (节省代码空间)
#define XY_CRYPTO_ENABLE_AES    1
#define XY_CRYPTO_ENABLE_SHA256 1
#define XY_CRYPTO_ENABLE_RSA    0  // RSA 较大,默认禁用
```

## 特性

- **哈希算法**
  - MD5
  - SHA-256
  - HMAC-MD5
  - HMAC-SHA256

- **对称加密**
  - AES-128/192/256 (ECB/CBC模式)

- **编码算法**
  - Base64 编解码
  - 十六进制 编解码

- **校验算法**
  - CRC32

- **其他功能**
  - 随机数生成
  - 跨平台支持 (Windows/Linux)

## 编译

### 使用 Make
```bash
make all          # 编译库和测试程序
make library      # 只编译库
make test         # 只编译测试程序
make run_test     # 运行测试
make clean        # 清理生成文件
```

### 使用 CMake
```bash
mkdir build
cd build
cmake ..
make
```

### Windows (Visual Studio)
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

## 使用示例

```c
#include "xy_tiny_crypto.h"
#include <stdio.h>
#include <string.h>

int main() {
    // MD5 哈希
    const char *data = "Hello, World!";
    uint8_t md5_digest[XY_MD5_DIGEST_SIZE];

    if (xy_md5_hash((const uint8_t*)data, strlen(data), md5_digest) == XY_CRYPTO_SUCCESS) {
        printf("MD5: ");
        for (int i = 0; i < XY_MD5_DIGEST_SIZE; i++) {
            printf("%02x", md5_digest[i]);
        }
        printf("\n");
    }

    // AES 加密
    uint8_t key[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                       0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    uint8_t plaintext[16] = "Hello, AES!     ";
    uint8_t ciphertext[16];

    xy_aes_ctx_t aes_ctx;
    xy_aes_init(&aes_ctx, key, XY_AES_KEY_SIZE_128);
    xy_aes_encrypt_block(&aes_ctx, plaintext, ciphertext);

    // Base64 编码
    char b64_output[64];
    xy_base64_encode((const uint8_t*)data, strlen(data), b64_output, sizeof(b64_output));
    printf("Base64: %s\n", b64_output);

    return 0;
}
```

## API 参考

### 哈希算法

#### MD5
```c
int xy_md5_hash(const uint8_t *data, size_t len, uint8_t digest[XY_MD5_DIGEST_SIZE]);
int xy_md5_init(xy_md5_ctx_t *ctx);
int xy_md5_update(xy_md5_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_md5_final(xy_md5_ctx_t *ctx, uint8_t digest[XY_MD5_DIGEST_SIZE]);
```

#### SHA-256
```c
int xy_sha256_hash(const uint8_t *data, size_t len, uint8_t digest[XY_SHA256_DIGEST_SIZE]);
int xy_sha256_init(xy_sha256_ctx_t *ctx);
int xy_sha256_update(xy_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
int xy_sha256_final(xy_sha256_ctx_t *ctx, uint8_t digest[XY_SHA256_DIGEST_SIZE]);
```

### 对称加密

#### AES
```c
int xy_aes_init(xy_aes_ctx_t *ctx, const uint8_t *key, int key_size);
int xy_aes_encrypt_block(xy_aes_ctx_t *ctx, const uint8_t *plaintext, uint8_t *ciphertext);
int xy_aes_decrypt_block(xy_aes_ctx_t *ctx, const uint8_t *ciphertext, uint8_t *plaintext);
int xy_aes_cbc_encrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *plaintext, size_t len, uint8_t *ciphertext);
int xy_aes_cbc_decrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *ciphertext, size_t len, uint8_t *plaintext);
```

### 编码算法

#### Base64
```c
int xy_base64_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len);
int xy_base64_decode(const char *input, size_t input_len, uint8_t *output, size_t output_len);
size_t xy_base64_encode_len(size_t input_len);
size_t xy_base64_decode_len(size_t input_len);
```

#### 十六进制
```c
int xy_hex_encode(const uint8_t *input, size_t input_len, char *output, size_t output_len);
int xy_hex_decode(const char *input, size_t input_len, uint8_t *output, size_t output_len);
size_t xy_hex_encode_len(size_t input_len);
size_t xy_hex_decode_len(size_t input_len);
```

### 其他功能

#### CRC32
```c
uint32_t xy_crc32(const uint8_t *data, size_t len);
uint32_t xy_crc32_update(uint32_t crc, const uint8_t *data, size_t len);
```

#### 随机数
```c
int xy_random_bytes(uint8_t *buffer, size_t len);
uint32_t xy_random_uint32(void);
```

#### HMAC
```c
int xy_hmac_md5(const uint8_t *key, size_t key_len,
                const uint8_t *data, size_t data_len,
                uint8_t digest[XY_MD5_DIGEST_SIZE]);
int xy_hmac_sha256(const uint8_t *key, size_t key_len,
                   const uint8_t *data, size_t data_len,
                   uint8_t digest[XY_SHA256_DIGEST_SIZE]);
```

## 返回值

- `XY_CRYPTO_SUCCESS` (0) - 成功
- `XY_CRYPTO_ERROR` (-1) - 一般错误
- `XY_CRYPTO_INVALID_PARAM` (-2) - 无效参数
- `XY_CRYPTO_BUFFER_TOO_SMALL` (-3) - 缓冲区太小

## 文件结构

```
xy_tiny_crypto/
├── include/
│   └── xy_tiny_crypto.h     # 主头文件
├── src/
│   ├── xy_md5.c            # MD5 实现
│   ├── xy_sha256.c         # SHA256 实现
│   ├── xy_aes.c            # AES 实现
│   ├── xy_base64.c         # Base64 实现
│   ├── xy_hex.c            # 十六进制实现
│   ├── xy_crc32.c          # CRC32 实现
│   ├── xy_random.c         # 随机数实现
│   └── xy_hmac.c           # HMAC 实现
├── test/
│   └── test_crypto.c       # 测试程序
├── Makefile                # Make 构建文件
├── CMakeLists.txt          # CMake 构建文件
└── ReadMe.md               # 说明文档
```

## 注意事项

1. 该库仅用于学习和轻量级应用，不建议在高安全要求的生产环境中使用
2. 随机数生成器在无法访问系统熵源时会降级为伪随机数生成器
3. AES 实现未包含填充方案，需要用户自行处理数据对齐
4. 所有函数都进行了基本的参数验证

## 许可证

MIT License

## 贡献

欢迎提交 Issues 和 Pull Requests！
