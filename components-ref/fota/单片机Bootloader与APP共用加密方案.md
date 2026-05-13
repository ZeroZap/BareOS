这是一个非常经典且重要的嵌入式系统设计问题。将Bootloader和APP的共用代码（尤其是加密/解密算法）提取出来以节省存储空间，是完全可行且推荐的做法。

核心思想是：**将共用的代码和数据封装成一个独立的“公共库”，这个库在物理存储上只存在一份，但Bootloader和APP都可以调用它。**

以下是详细的实施步骤、关键技术和注意事项：

### 一、 整体架构设计

你需要将Flash存储器划分为三个主要区域：

1. **Bootloader区**：存储Bootloader核心代码（如初始化、通信协议、更新逻辑等）。
2. **APP区**：存储用户应用程序。
3. **Common Library区（核心）**：存储Bootloader和APP共用的代码和数据。这通常是**加密/解密算法**（如AES, SHA, RSA等）、CRC校验函数、以及可能共用的底层驱动（如Flash驱动）。

text

```
+---------------------+  Flash Memory Map
|      Bootloader     |  (固定位置，例如 0x0800 0000)
+---------------------+
|   Common Library    |  (固定位置，例如 0x0800 C000)
+---------------------+
|         APP         |  (固定位置，例如 0x0801 0000)
+---------------------+
```



### 二、 关键技术实施细节

#### 1. 确定共用内容

- **加密/解密算法**：这是最主要的共用部分。例如AES的`Encrypt`和`Decrypt`函数。
- **密钥数据**：如果使用对称加密，密钥可以存储在这个公共区域。**注意安全风险**。
- **CRC校验函数**：用于验证APP固件完整性的CRC计算函数。
- **公共工具函数**：如内存操作（`memcpy`, `memset`）、字符串操作等。

#### 2. 固定Common Library的链接地址

这是最关键的一步。你必须在链接脚本中，将Common Library的代码和数据固定链接到Flash的特定地址。

- **修改链接脚本**：

  - 为Bootloader、Common Library和APP分别创建独立的链接脚本，或者在一个脚本中明确定义各个段的位置。
  - 在Common Library的链接脚本中，指定其`.text`（代码）和`.rodata`（只读数据）段的起始地址为你规划的`Common Library`区域地址（例如 `0x0800 C000`）。

  **示例 (GCC LD Script片段)：**

  ld

  ```
  /* Bootloader 链接脚本 */
  MEMORY
  {
      RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
      FLASH (rx) : ORIGIN = 0x8000000,  LENGTH = 48K  /* Bootloader 占 48K */
  }
  
  /* Common Library 链接脚本 */
  MEMORY
  {
      RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
      FLASH (rx) : ORIGIN = 0x800C000,  LENGTH = 16K  /* 公共库固定放在 0x800C000 */
  }
  
  /* APP 链接脚本 */
  MEMORY
  {
      RAM (xrw)  : ORIGIN = 0x20000000, LENGTH = 128K
      FLASH (rx) : ORIGIN = 0x8010000,  LENGTH = 192K /* APP 从 0x8010000 开始 */
  }
  ```

  

#### 3. 创建函数跳转表/接口

Bootloader和APP不能直接以“函数调用”的方式使用Common Library，因为它们的代码是独立编译的，链接地址不同。解决方案是使用**函数指针表**。

- **在Common Library中定义一个结构体**，包含所有公共函数的指针。

  c

  ```
  // common_interface.h
  #ifndef COMMON_INTERFACE_H
  #define COMMON_INTERFACE_H
  
  #include <stdint.h>
  
  // 定义函数指针类型
  typedef void (*aes_encrypt_t)(const uint8_t* input, uint8_t* output, const uint8_t* key);
  typedef uint32_t (*calculate_crc_t)(const uint8_t* data, uint32_t length);
  
  // 定义函数表结构体
  typedef struct {
      aes_encrypt_t aes_encrypt;
      calculate_crc_t calculate_crc;
      // ... 添加其他共用函数
  } common_lib_t;
  
  // 声明一个外部的函数表，该表在Common Library中定义
  extern const common_lib_t CommonLib;
  
  #endif
  ```

  

- **在Common Library的代码中实例化这个结构体**：

  c

  ```
  // common_lib.c
  #include "common_interface.h"
  
  static void my_aes_encrypt(const uint8_t* input, uint8_t* output, const uint8_t* key) {
      // 你的AES加密实现
  }
  
  static uint32_t my_calculate_crc(const uint8_t* data, uint32_t length) {
      // 你的CRC计算实现
  }
  
  // 在固定地址处初始化函数表
  const common_lib_t CommonLib __attribute__((section(".common_lib_section"))) = {
      .aes_encrypt = my_aes_encrypt,
      .calculate_crc = my_calculate_crc,
  };
  ```

  

  使用`__attribute__((section(...)))`确保这个结构体被链接到你指定的固定地址。

#### 4. Bootloader和APP的调用方式

现在，Bootloader和APP可以通过访问这个固定的地址来调用公共函数。

- **在Bootloader/APP中**：

  c

  ```
  #include "common_interface.h"
  
  // 直接使用
  void bootloader_task(void) {
      uint8_t plaintext[16] = {...};
      uint8_t ciphertext[16];
      uint8_t key[16] = {...};
  
      // 通过函数表调用公共库函数
      CommonLib.aes_encrypt(plaintext, ciphertext, key);
  
      uint32_t crc = CommonLib.calculate_crc(firmware_data, firmware_length);
  }
  ```

  

  为了让编译器能找到`CommonLib`，你需要在Bootloader和APP的工程中：

  - 包含 `common_interface.h` 头文件。
  - 在链接配置中，**告诉链接器 `CommonLib` 已经在外部（`0x0800 C000` + 结构体偏移）定义好了**，不需要在本工程内链接。通常可以通过声明为`extern const`并妥善处理链接警告来实现。

#### 5. 加密流程配合

整个加密升级流程如下：

1. **发布端**：使用相同的加密算法和密钥对APP固件（`.bin`或`.hex`）进行加密。
2. **传输**：将加密后的固件发送给设备。
3. **Bootloader**：
   - 接收加密固件，并存入临时区域。
   - **调用Common Library中的解密函数**，将固件解密到临时缓冲区或直接解密到APP区的目标地址（如果支持）。
   - 计算解密后数据的CRC，与预期的CRC（可能随固件一起传输）进行比对。
   - 校验通过后，跳转到APP入口地址。
4. **APP**：如果需要验证自身完整性或与服务器进行安全通信，同样可以**调用Common Library中的加密/解密函数**。

### 三、 优点与缺点

#### 优点：

- **显著节省Flash空间**：避免了同一套算法代码的两份拷贝。
- **维护方便**：加密算法升级时，只需更新Common Library一处。
- **保证一致性**：Bootloader和APP使用完全相同的算法，避免了因编译差异导致的潜在问题。

#### 缺点和挑战：

- **增加复杂性**：链接脚本、函数表的设计增加了系统设计的复杂度。
- **耦合性**：Common Library的接口一旦确定，后续修改需要同时考虑Bootloader和APP的兼容性。
- **调试难度**：如果公共库函数出现问题，需要同时调试Bootloader和APP两个工程。
- **安全风险**：密钥存储在固定的Flash位置，如果该区域可被APP访问，则增加了密钥被泄露的风险。可以考虑使用芯片的硬件加密模块或安全存储区域来保护密钥。

### 四、 替代方案与进阶考虑

- **使用硬件加密引擎**：如果MCU自带硬件加密（如AES加速器），Bootloader和APP都直接操作硬件寄存器，自然就实现了“共用”，这是最推荐的方式。
- **将库函数编译为位置无关代码**：这样库可以被加载到任意地址执行。但通常效率稍低，且实现起来同样复杂。
- **使用RTOS的共享库机制**：在一些高级RTOS中，有动态加载模块的机制，但资源占用较多，不适合低端单片机。

**总结**：
实施Bootloader与APP共用加密库的方案，技术上是完全可行的，核心在于**通过链接脚本固定公共库地址**，并**通过函数指针表建立调用接口**。在项目初期进行精心设计，可以带来存储空间节省和良好维护性的收益，但也要充分考虑其带来的复杂度和安全风险。对于新项目，优先考虑使用硬件加密引擎是更优的选择。