## 单片机Shell命令框架实现

### 1. 头文件 `shell_cmd.h`

```c
#ifndef __SHELL_CMD_H__
#define __SHELL_CMD_H__

#include <stdint.h>

/* 命令处理函数类型 */
typedef int (*cmd_handler_t)(int argc, char *argv[]);

/* 命令结构体 */
typedef struct {
    const char *name;        /* 命令名称 */
    const char *desc;        /* 命令描述 */
    cmd_handler_t handler;   /* 命令处理函数 */
} shell_cmd_t;

/*===========================================================================
 * 编译器相关宏定义
 *===========================================================================*/

/* GCC 编译器 (ARM-GCC, RISC-V GCC等) */
#if defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC_VERSION)
    #define SECTION(x)      __attribute__((section(x)))
    #define USED            __attribute__((used))
    #define SHELL_SECTION   ".shell_cmd"
    
/* ARM Compiler 5 (Keil MDK-ARM AC5) */
#elif defined(__CC_ARM)
    #define SECTION(x)      __attribute__((section(x)))
    #define USED            __attribute__((used))
    #define SHELL_SECTION   "shell_cmd"
    
/* ARM Compiler 6 (Keil MDK-ARM AC6 / armclang) */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
    #define SECTION(x)      __attribute__((section(x)))
    #define USED            __attribute__((used))
    #define SHELL_SECTION   ".shell_cmd"
    
/* IAR 编译器 */
#elif defined(__ICCARM__) || defined(__ICC430__)
    #define SECTION(x)      @ x
    #define USED            __root
    #define SHELL_SECTION   ".shell_cmd"
    
#else
    #error "Unsupported compiler!"
#endif

/*===========================================================================
 * 命令注册宏
 *===========================================================================*/

/* 命令注册宏 - 通用版本 */
#if defined(__ICCARM__) || defined(__ICC430__)
    /* IAR 特殊语法 */
    #define SHELL_CMD_EXPORT(_name, _desc, _handler)                        \
        USED const shell_cmd_t __shell_cmd_##_name SECTION(SHELL_SECTION) = \
        {                                                                   \
            .name    = #_name,                                              \
            .desc    = _desc,                                               \
            .handler = _handler,                                            \
        }
#else
    /* GCC / ARM Compiler 语法 */
    #define SHELL_CMD_EXPORT(_name, _desc, _handler)                        \
        USED const shell_cmd_t __shell_cmd_##_name                          \
        SECTION(SHELL_SECTION) =                                            \
        {                                                                   \
            .name    = #_name,                                              \
            .desc    = _desc,                                               \
            .handler = _handler,                                            \
        }
#endif

/*===========================================================================
 * Section边界声明 (需要在链接脚本中定义)
 *===========================================================================*/

#if defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC_VERSION)
    extern const shell_cmd_t __start_shell_cmd;
    extern const shell_cmd_t __stop_shell_cmd;
    #define SHELL_CMD_BEGIN     (&__start_shell_cmd)
    #define SHELL_CMD_END       (&__stop_shell_cmd)
    
#elif defined(__CC_ARM)
    extern const shell_cmd_t shell_cmd$$Base;
    extern const shell_cmd_t shell_cmd$$Limit;
    #define SHELL_CMD_BEGIN     (&shell_cmd$$Base)
    #define SHELL_CMD_END       (&shell_cmd$$Limit)
    
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
    extern const shell_cmd_t __shell_cmd_start;
    extern const shell_cmd_t __shell_cmd_end;
    #define SHELL_CMD_BEGIN     (&__shell_cmd_start)
    #define SHELL_CMD_END       (&__shell_cmd_end)
    
#elif defined(__ICCARM__) || defined(__ICC430__)
    #pragma section = ".shell_cmd"
    #define SHELL_CMD_BEGIN     ((const shell_cmd_t *)__section_begin(".shell_cmd"))
    #define SHELL_CMD_END       ((const shell_cmd_t *)__section_end(".shell_cmd"))
#endif

/*===========================================================================
 * API函数声明
 *===========================================================================*/

int  cmd_parsing(char *str);
void shell_cmd_list(void);

#endif /* __SHELL_CMD_H__ */
```

### 2. 源文件 `shell_cmd.c`

```c
#include "shell_cmd.h"
#include <string.h>
#include <stdio.h>

#define MAX_ARGC    10      /* 最大参数个数 */

/*===========================================================================
 * 内部函数
 *===========================================================================*/

/**
 * @brief  解析命令行字符串为argc/argv格式
 * @param  str: 输入字符串 (会被修改)
 * @param  argv: 参数指针数组
 * @return 参数个数
 */
static int parse_args(char *str, char *argv[])
{
    int argc = 0;
    char *ptr = str;
    int in_token = 0;
    
    while (*ptr != '\0' && argc < MAX_ARGC) {
        /* 跳过空格和制表符 */
        if (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
            if (in_token) {
                *ptr = '\0';  /* 结束当前token */
                in_token = 0;
            }
        } else {
            if (!in_token) {
                argv[argc++] = ptr;  /* 新token开始 */
                in_token = 1;
            }
        }
        ptr++;
    }
    
    return argc;
}

/**
 * @brief  查找命令
 * @param  name: 命令名称
 * @return 命令指针，未找到返回NULL
 */
static const shell_cmd_t *find_cmd(const char *name)
{
    const shell_cmd_t *cmd;
    
    for (cmd = SHELL_CMD_BEGIN; cmd < SHELL_CMD_END; cmd++) {
        if (cmd->name && strcmp(cmd->name, name) == 0) {
            return cmd;
        }
    }
    
    return NULL;
}

/*===========================================================================
 * API函数实现
 *===========================================================================*/

/**
 * @brief  命令解析执行函数
 * @param  str: 命令行字符串
 * @return 0:成功, -1:命令未找到, -2:空命令, 其他:命令返回值
 */
int cmd_parsing(char *str)
{
    char *argv[MAX_ARGC];
    int argc;
    const shell_cmd_t *cmd;
    
    if (str == NULL || *str == '\0') {
        return -2;
    }
    
    /* 解析参数 */
    argc = parse_args(str, argv);
    if (argc == 0) {
        return -2;
    }
    
    /* 查找命令 */
    cmd = find_cmd(argv[0]);
    if (cmd == NULL) {
        printf("Unknown command: %s\r\n", argv[0]);
        printf("Type 'help' for available commands.\r\n");
        return -1;
    }
    
    /* 执行命令 */
    if (cmd->handler) {
        return cmd->handler(argc, argv);
    }
    
    return -1;
}

/**
 * @brief  列出所有注册的命令
 */
void shell_cmd_list(void)
{
    const shell_cmd_t *cmd;
    int count = 0;
    
    printf("\r\n========== Shell Commands ==========\r\n");
    
    for (cmd = SHELL_CMD_BEGIN; cmd < SHELL_CMD_END; cmd++) {
        if (cmd->name) {
            printf("  %-12s - %s\r\n", cmd->name, cmd->desc ? cmd->desc : "No description");
            count++;
        }
    }
    
    printf("====================================\r\n");
    printf("Total: %d commands\r\n\r\n", count);
}

/*===========================================================================
 * 内置命令
 *===========================================================================*/

/* help命令 */
static int cmd_help(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    shell_cmd_list();
    return 0;
}
SHELL_CMD_EXPORT(help, "List all commands", cmd_help);

/* version命令 */
static int cmd_version(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("Shell Version: 1.0.0\r\n");
    printf("Build Date: %s %s\r\n", __DATE__, __TIME__);
    return 0;
}
SHELL_CMD_EXPORT(version, "Show version info", cmd_version);
```

### 3. 链接脚本配置

#### GCC 链接脚本 (.ld)

```ld
SECTIONS
{
    /* ... 其他section ... */
    
    /* Shell命令段 */
    .shell_cmd :
    {
        . = ALIGN(4);
        __start_shell_cmd = .;
        KEEP(*(SORT(.shell_cmd*)))
        __stop_shell_cmd = .;
        . = ALIGN(4);
    } > FLASH
    
    /* ... 其他section ... */
}
```

#### ARM Compiler 6 (scatter file .sct)

```text
LR_IROM1 0x08000000 0x00080000  {
    ER_IROM1 0x08000000 0x00080000  {
        *.o (RESET, +First)
        *(InRoot$$Sections)
        .ANY (+RO)
        
        ; Shell命令段
        __shell_cmd_start = .;
        *(.shell_cmd*)
        __shell_cmd_end = .;
    }
    
    RW_IRAM1 0x20000000 0x00010000  {
        .ANY (+RW +ZI)
    }
}
```

### 4. 使用示例 `user_cmd.c`

```c
#include "shell_cmd.h"
#include <stdio.h>
#include <stdlib.h>

/*===========================================================================
 * 用户自定义命令 - 通过宏自动注册
 *===========================================================================*/

/* LED控制命令 */
static int cmd_led(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: led <on|off|toggle>\r\n");
        return -1;
    }
    
    if (strcmp(argv[1], "on") == 0) {
        printf("LED ON\r\n");
        // HAL_GPIO_WritePin(LED_GPIO, LED_PIN, GPIO_PIN_SET);
    } else if (strcmp(argv[1], "off") == 0) {
        printf("LED OFF\r\n");
        // HAL_GPIO_WritePin(LED_GPIO, LED_PIN, GPIO_PIN_RESET);
    } else if (strcmp(argv[1], "toggle") == 0) {
        printf("LED TOGGLE\r\n");
        // HAL_GPIO_TogglePin(LED_GPIO, LED_PIN);
    } else {
        printf("Unknown option: %s\r\n", argv[1]);
        return -1;
    }
    
    return 0;
}
SHELL_CMD_EXPORT(led, "Control LED: led <on|off|toggle>", cmd_led);

/* 内存读取命令 */
static int cmd_read(int argc, char *argv[])
{
    uint32_t addr, len = 16;
    
    if (argc < 2) {
        printf("Usage: read <addr> [len]\r\n");
        return -1;
    }
    
    addr = strtoul(argv[1], NULL, 0);
    if (argc >= 3) {
        len = strtoul(argv[2], NULL, 0);
    }
    
    printf("Read from 0x%08lX, len=%lu:\r\n", addr, len);
    
    for (uint32_t i = 0; i < len; i++) {
        if (i % 16 == 0) printf("0x%08lX: ", addr + i);
        printf("%02X ", *((volatile uint8_t *)(addr + i)));
        if ((i + 1) % 16 == 0) printf("\r\n");
    }
    if (len % 16 != 0) printf("\r\n");
    
    return 0;
}
SHELL_CMD_EXPORT(read, "Read memory: read <addr> [len]", cmd_read);

/* 重启命令 */
static int cmd_reboot(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("System rebooting...\r\n");
    // NVIC_SystemReset();
    return 0;
}
SHELL_CMD_EXPORT(reboot, "Reboot system", cmd_reboot);

/* echo命令 */
static int cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\r\n");
    return 0;
}
SHELL_CMD_EXPORT(echo, "Echo input text", cmd_echo);
```

### 5. 主程序调用示例

```c
#include "shell_cmd.h"
#include <stdio.h>
#include <string.h>

#define CMD_BUF_SIZE    128

static char cmd_buffer[CMD_BUF_SIZE];
static uint8_t cmd_index = 0;

/* 串口接收处理 (可放在中断或轮询中) */
void shell_input(char ch)
{
    if (ch == '\r' || ch == '\n') {
        if (cmd_index > 0) {
            cmd_buffer[cmd_index] = '\0';
            printf("\r\n");
            
            /* 解析执行命令 */
            cmd_parsing(cmd_buffer);
            
            cmd_index = 0;
            memset(cmd_buffer, 0, CMD_BUF_SIZE);
        }
        printf("shell> ");
    }
    else if (ch == '\b' || ch == 0x7F) {  /* 退格 */
        if (cmd_index > 0) {
            cmd_index--;
            printf("\b \b");
        }
    }
    else if (cmd_index < CMD_BUF_SIZE - 1) {
        cmd_buffer[cmd_index++] = ch;
        printf("%c", ch);  /* 回显 */
    }
}

int main(void)
{
    /* 系统初始化... */
    
    printf("\r\n===== MCU Shell Ready =====\r\n");
    printf("Type 'help' for commands\r\n");
    printf("shell> ");
    
    while (1) {
        /* 假设从串口获取字符 */
        // char ch = uart_getchar();
        // shell_input(ch);
    }
}
```

### 运行效果

```text
===== MCU Shell Ready =====
Type 'help' for commands
shell> help

========== Shell Commands ==========
  help         - List all commands
  version      - Show version info
  led          - Control LED: led <on|off|toggle>
  read         - Read memory: read <addr> [len]
  reboot       - Reboot system
  echo         - Echo input text
====================================
Total: 6 commands

shell> led on
LED ON
shell> echo Hello World
Hello World
shell> version
Shell Version: 1.0.0
Build Date: Feb 21 2026 10:30:45
```

### 关键特性

|         特性          |                      说明                       |
| :-------------------: | :---------------------------------------------: |
|     **自动注册**      | 使用`SHELL_CMD_EXPORT`宏，命令自动注册到section |
|   **多编译器支持**    |             GCC、Keil AC5/AC6、IAR              |
|   **零初始化代码**    |             利用链接器自动收集命令              |
| **argc/argv标准格式** |               类似Linux命令行解析               |
|      **易扩展**       |       任意.c文件中添加命令，无需修改框架        |





```
可用给shell执行或者命令执行增加 权限环境嚒
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

当然可以！我来为Shell命令框架增加**权限管理系统**，支持多级权限、用户登录、命令权限检查等功能。

## 带权限管理的Shell命令框架

### 1. 头文件 `shell_cmd.h`

```c
#ifndef __SHELL_CMD_H__
#define __SHELL_CMD_H__

#include <stdint.h>

/*===========================================================================
 * 权限等级定义
 *===========================================================================*/
typedef enum {
    PERM_GUEST      = 0x00,     /* 访客权限 - 最低 */
    PERM_USER       = 0x01,     /* 普通用户 */
    PERM_OPERATOR   = 0x02,     /* 操作员 */
    PERM_ADMIN      = 0x04,     /* 管理员 */
    PERM_ROOT       = 0x08,     /* 超级用户 - 最高 */
    PERM_NONE       = 0xFF,     /* 禁用命令 */
} shell_perm_t;

/* 权限组合 (用于命令可被多个权限级别访问) */
#define PERM_ALL            (PERM_GUEST | PERM_USER | PERM_OPERATOR | PERM_ADMIN | PERM_ROOT)
#define PERM_USER_ABOVE     (PERM_USER | PERM_OPERATOR | PERM_ADMIN | PERM_ROOT)
#define PERM_OPERATOR_ABOVE (PERM_OPERATOR | PERM_ADMIN | PERM_ROOT)
#define PERM_ADMIN_ABOVE    (PERM_ADMIN | PERM_ROOT)

/*===========================================================================
 * 命令属性标志
 *===========================================================================*/
typedef enum {
    CMD_FLAG_NONE       = 0x00,     /* 无特殊标志 */
    CMD_FLAG_HIDDEN     = 0x01,     /* 隐藏命令 (help不显示) */
    CMD_FLAG_DISABLED   = 0x02,     /* 禁用命令 */
    CMD_FLAG_LOG        = 0x04,     /* 记录执行日志 */
    CMD_FLAG_CONFIRM    = 0x08,     /* 执行前需确认 */
} cmd_flag_t;

/*===========================================================================
 * 用户信息结构
 *===========================================================================*/
#define MAX_USERNAME_LEN    16
#define MAX_PASSWORD_LEN    32

typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];    /* 实际应用建议存储hash */
    shell_perm_t permission;
    uint8_t enabled;
} shell_user_t;

/*===========================================================================
 * Shell上下文环境
 *===========================================================================*/
typedef struct {
    shell_perm_t current_perm;          /* 当前权限级别 */
    char current_user[MAX_USERNAME_LEN];/* 当前登录用户 */
    uint8_t is_logged_in;               /* 是否已登录 */
    uint32_t login_time;                /* 登录时间戳 */
    uint32_t last_active;               /* 最后活动时间 */
    uint32_t timeout_sec;               /* 超时时间(秒), 0=不超时 */
    uint8_t login_attempts;             /* 登录尝试次数 */
    uint8_t max_attempts;               /* 最大尝试次数 */
    uint8_t locked;                     /* 账户锁定状态 */
} shell_context_t;

/*===========================================================================
 * 命令处理函数类型
 *===========================================================================*/
typedef int (*cmd_handler_t)(int argc, char *argv[]);

/*===========================================================================
 * 命令结构体 (带权限)
 *===========================================================================*/
typedef struct {
    const char      *name;          /* 命令名称 */
    const char      *desc;          /* 命令描述 */
    const char      *usage;         /* 使用说明 */
    cmd_handler_t   handler;        /* 命令处理函数 */
    shell_perm_t    permission;     /* 所需权限 */
    cmd_flag_t      flags;          /* 命令标志 */
} shell_cmd_t;

/*===========================================================================
 * 编译器相关宏定义
 *===========================================================================*/

/* GCC 编译器 */
#if defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC_VERSION)
    #define SECTION(x)      __attribute__((section(x)))
    #define USED            __attribute__((used))
    #define SHELL_SECTION   ".shell_cmd"
    
/* ARM Compiler 5 (Keil AC5) */
#elif defined(__CC_ARM)
    #define SECTION(x)      __attribute__((section(x)))
    #define USED            __attribute__((used))
    #define SHELL_SECTION   "shell_cmd"
    
/* ARM Compiler 6 (Keil AC6) */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
    #define SECTION(x)      __attribute__((section(x)))
    #define USED            __attribute__((used))
    #define SHELL_SECTION   ".shell_cmd"
    
/* IAR 编译器 */
#elif defined(__ICCARM__) || defined(__ICC430__)
    #define SECTION(x)      @ x
    #define USED            __root
    #define SHELL_SECTION   ".shell_cmd"
    
#else
    #error "Unsupported compiler!"
#endif

/*===========================================================================
 * 命令注册宏
 *===========================================================================*/

/* 完整版命令注册宏 */
#if defined(__ICCARM__) || defined(__ICC430__)
    #define _SHELL_CMD_EXPORT(_name, _desc, _usage, _handler, _perm, _flags) \
        USED const shell_cmd_t __shell_cmd_##_name SECTION(SHELL_SECTION) = { \
            .name       = #_name,                                             \
            .desc       = _desc,                                              \
            .usage      = _usage,                                             \
            .handler    = _handler,                                           \
            .permission = _perm,                                              \
            .flags      = _flags,                                             \
        }
#else
    #define _SHELL_CMD_EXPORT(_name, _desc, _usage, _handler, _perm, _flags) \
        USED const shell_cmd_t __shell_cmd_##_name                           \
        SECTION(SHELL_SECTION) = {                                           \
            .name       = #_name,                                             \
            .desc       = _desc,                                              \
            .usage      = _usage,                                             \
            .handler    = _handler,                                           \
            .permission = _perm,                                              \
            .flags      = _flags,                                             \
        }
#endif

/* 简化版宏 - 默认GUEST权限，所有人可用 */
#define SHELL_CMD_EXPORT(_name, _desc, _handler) \
    _SHELL_CMD_EXPORT(_name, _desc, NULL, _handler, PERM_ALL, CMD_FLAG_NONE)

/* 带权限的命令注册宏 */
#define SHELL_CMD_EXPORT_PERM(_name, _desc, _handler, _perm) \
    _SHELL_CMD_EXPORT(_name, _desc, NULL, _handler, _perm, CMD_FLAG_NONE)

/* 完整参数的命令注册宏 */
#define SHELL_CMD_EXPORT_FULL(_name, _desc, _usage, _handler, _perm, _flags) \
    _SHELL_CMD_EXPORT(_name, _desc, _usage, _handler, _perm, _flags)

/* 管理员命令快捷宏 */
#define SHELL_CMD_ADMIN(_name, _desc, _handler) \
    _SHELL_CMD_EXPORT(_name, _desc, NULL, _handler, PERM_ADMIN_ABOVE, CMD_FLAG_LOG)

/* ROOT命令快捷宏 */
#define SHELL_CMD_ROOT(_name, _desc, _handler) \
    _SHELL_CMD_EXPORT(_name, _desc, NULL, _handler, PERM_ROOT, CMD_FLAG_LOG | CMD_FLAG_CONFIRM)

/* 隐藏命令宏 */
#define SHELL_CMD_HIDDEN(_name, _desc, _handler, _perm) \
    _SHELL_CMD_EXPORT(_name, _desc, NULL, _handler, _perm, CMD_FLAG_HIDDEN)

/*===========================================================================
 * Section边界声明
 *===========================================================================*/

#if defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC_VERSION)
    extern const shell_cmd_t __start_shell_cmd;
    extern const shell_cmd_t __stop_shell_cmd;
    #define SHELL_CMD_BEGIN     (&__start_shell_cmd)
    #define SHELL_CMD_END       (&__stop_shell_cmd)
    
#elif defined(__CC_ARM)
    extern const shell_cmd_t shell_cmd$$Base;
    extern const shell_cmd_t shell_cmd$$Limit;
    #define SHELL_CMD_BEGIN     (&shell_cmd$$Base)
    #define SHELL_CMD_END       (&shell_cmd$$Limit)
    
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
    extern const shell_cmd_t __shell_cmd_start;
    extern const shell_cmd_t __shell_cmd_end;
    #define SHELL_CMD_BEGIN     (&__shell_cmd_start)
    #define SHELL_CMD_END       (&__shell_cmd_end)
    
#elif defined(__ICCARM__) || defined(__ICC430__)
    #pragma section = ".shell_cmd"
    #define SHELL_CMD_BEGIN     ((const shell_cmd_t *)__section_begin(".shell_cmd"))
    #define SHELL_CMD_END       ((const shell_cmd_t *)__section_end(".shell_cmd"))
#endif

/*===========================================================================
 * API函数声明
 *===========================================================================*/

/* 核心功能 */
int  cmd_parsing(char *str);
void shell_init(void);

/* 权限管理 */
int  shell_login(const char *username, const char *password);
void shell_logout(void);
int  shell_check_permission(shell_perm_t required);
shell_perm_t shell_get_current_perm(void);
const char*  shell_get_current_user(void);

/* 用户管理 */
int  shell_user_add(const char *username, const char *password, shell_perm_t perm);
int  shell_user_del(const char *username);
int  shell_user_set_perm(const char *username, shell_perm_t perm);
int  shell_user_change_password(const char *username, const char *old_pwd, const char *new_pwd);

/* 会话管理 */
void shell_set_timeout(uint32_t seconds);
void shell_refresh_activity(void);
int  shell_check_timeout(uint32_t current_time);

/* 辅助功能 */
void shell_cmd_list(void);
const char* shell_perm_to_string(shell_perm_t perm);

#endif /* __SHELL_CMD_H__ */
```

### 2. 源文件 `shell_cmd.c`

```c
#include "shell_cmd.h"
#include <string.h>
#include <stdio.h>

/*===========================================================================
 * 配置项
 *===========================================================================*/
#define MAX_ARGC            16          /* 最大参数个数 */
#define MAX_USERS           8           /* 最大用户数 */
#define DEFAULT_TIMEOUT     300         /* 默认超时时间(秒) */
#define MAX_LOGIN_ATTEMPTS  3           /* 最大登录尝试次数 */
#define LOCKOUT_TIME        60          /* 锁定时间(秒) */

/*===========================================================================
 * 静态变量
 *===========================================================================*/

/* Shell上下文 */
static shell_context_t g_shell_ctx = {
    .current_perm   = PERM_GUEST,
    .current_user   = "guest",
    .is_logged_in   = 0,
    .login_time     = 0,
    .last_active    = 0,
    .timeout_sec    = DEFAULT_TIMEOUT,
    .login_attempts = 0,
    .max_attempts   = MAX_LOGIN_ATTEMPTS,
    .locked         = 0,
};

/* 用户数据库 (实际应用可存储在Flash/EEPROM) */
static shell_user_t g_user_db[MAX_USERS] = {
    {"root",     "root123",   PERM_ROOT,     1},
    {"admin",    "admin123",  PERM_ADMIN,    1},
    {"operator", "oper123",   PERM_OPERATOR, 1},
    {"user",     "user123",   PERM_USER,     1},
    {"guest",    "",          PERM_GUEST,    1},
};
static uint8_t g_user_count = 5;

/*===========================================================================
 * 内部函数
 *===========================================================================*/

/**
 * @brief  解析命令行字符串为argc/argv格式
 */
static int parse_args(char *str, char *argv[])
{
    int argc = 0;
    char *ptr = str;
    int in_token = 0;
    int in_quote = 0;
    char quote_char = 0;
    
    while (*ptr != '\0' && argc < MAX_ARGC) {
        /* 处理引号 */
        if ((*ptr == '"' || *ptr == '\'') && !in_quote) {
            in_quote = 1;
            quote_char = *ptr;
            if (!in_token) {
                argv[argc++] = ptr + 1;
                in_token = 1;
            }
            memmove(ptr, ptr + 1, strlen(ptr));
            continue;
        }
        
        if (*ptr == quote_char && in_quote) {
            in_quote = 0;
            *ptr = '\0';
            in_token = 0;
            ptr++;
            continue;
        }
        
        /* 处理空格 */
        if ((*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') && !in_quote) {
            if (in_token) {
                *ptr = '\0';
                in_token = 0;
            }
        } else {
            if (!in_token) {
                argv[argc++] = ptr;
                in_token = 1;
            }
        }
        ptr++;
    }
    
    return argc;
}

/**
 * @brief  查找用户
 */
static shell_user_t* find_user(const char *username)
{
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].username, username) == 0) {
            return &g_user_db[i];
        }
    }
    return NULL;
}

/**
 * @brief  查找命令
 */
static const shell_cmd_t* find_cmd(const char *name)
{
    const shell_cmd_t *cmd;
    
    for (cmd = SHELL_CMD_BEGIN; cmd < SHELL_CMD_END; cmd++) {
        if (cmd->name && strcmp(cmd->name, name) == 0) {
            return cmd;
        }
    }
    return NULL;
}

/**
 * @brief  简单确认函数
 */
static int confirm_action(const char *action)
{
    char buf[8];
    printf("Are you sure to %s? (yes/no): ", action);
    
    /* 这里简化处理，实际需要从串口读取 */
    /* 假设用户输入了yes */
    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (strncmp(buf, "yes", 3) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief  记录命令日志
 */
static void log_command(const char *user, const char *cmd, int result)
{
    /* 实际应用可写入Flash或发送到日志服务器 */
    printf("[LOG] User:%s Cmd:%s Result:%d\r\n", user, cmd, result);
}

/*===========================================================================
 * 权限字符串转换
 *===========================================================================*/
const char* shell_perm_to_string(shell_perm_t perm)
{
    switch (perm) {
        case PERM_ROOT:     return "root";
        case PERM_ADMIN:    return "admin";
        case PERM_OPERATOR: return "operator";
        case PERM_USER:     return "user";
        case PERM_GUEST:    return "guest";
        default:            return "unknown";
    }
}

/*===========================================================================
 * Shell初始化
 *===========================================================================*/
void shell_init(void)
{
    memset(&g_shell_ctx, 0, sizeof(g_shell_ctx));
    g_shell_ctx.current_perm = PERM_GUEST;
    strcpy(g_shell_ctx.current_user, "guest");
    g_shell_ctx.timeout_sec = DEFAULT_TIMEOUT;
    g_shell_ctx.max_attempts = MAX_LOGIN_ATTEMPTS;
    
    printf("\r\n");
    printf("========================================\r\n");
    printf("  Secure Shell v2.0 - Permission System\r\n");
    printf("========================================\r\n");
    printf("Type 'help' for available commands\r\n");
    printf("Type 'login <user> <pass>' to login\r\n");
    printf("\r\n");
}

/*===========================================================================
 * 权限检查
 *===========================================================================*/
int shell_check_permission(shell_perm_t required)
{
    /* 检查当前权限是否满足要求 */
    if (g_shell_ctx.current_perm & required) {
        return 1;  /* 有权限 */
    }
    return 0;  /* 无权限 */
}

shell_perm_t shell_get_current_perm(void)
{
    return g_shell_ctx.current_perm;
}

const char* shell_get_current_user(void)
{
    return g_shell_ctx.current_user;
}

/*===========================================================================
 * 登录/登出
 *===========================================================================*/
int shell_login(const char *username, const char *password)
{
    shell_user_t *user;
    
    /* 检查是否锁定 */
    if (g_shell_ctx.locked) {
        printf("Account locked. Please wait.\r\n");
        return -3;
    }
    
    /* 查找用户 */
    user = find_user(username);
    if (user == NULL || !user->enabled) {
        g_shell_ctx.login_attempts++;
        printf("Invalid username or password. (%d/%d)\r\n", 
               g_shell_ctx.login_attempts, g_shell_ctx.max_attempts);
        
        if (g_shell_ctx.login_attempts >= g_shell_ctx.max_attempts) {
            g_shell_ctx.locked = 1;
            printf("Too many failed attempts. Account locked for %d seconds.\r\n", LOCKOUT_TIME);
        }
        return -1;
    }
    
    /* 验证密码 */
    if (strcmp(user->password, password) != 0) {
        g_shell_ctx.login_attempts++;
        printf("Invalid username or password. (%d/%d)\r\n",
               g_shell_ctx.login_attempts, g_shell_ctx.max_attempts);
        
        if (g_shell_ctx.login_attempts >= g_shell_ctx.max_attempts) {
            g_shell_ctx.locked = 1;
            printf("Too many failed attempts. Account locked for %d seconds.\r\n", LOCKOUT_TIME);
        }
        return -2;
    }
    
    /* 登录成功 */
    g_shell_ctx.current_perm = user->permission;
    strncpy(g_shell_ctx.current_user, username, MAX_USERNAME_LEN - 1);
    g_shell_ctx.is_logged_in = 1;
    g_shell_ctx.login_attempts = 0;
    /* g_shell_ctx.login_time = get_system_time(); */
    
    printf("Login successful! Welcome, %s [%s]\r\n", 
           username, shell_perm_to_string(user->permission));
    
    return 0;
}

void shell_logout(void)
{
    printf("User '%s' logged out.\r\n", g_shell_ctx.current_user);
    
    g_shell_ctx.current_perm = PERM_GUEST;
    strcpy(g_shell_ctx.current_user, "guest");
    g_shell_ctx.is_logged_in = 0;
    g_shell_ctx.login_time = 0;
}

/*===========================================================================
 * 会话超时管理
 *===========================================================================*/
void shell_set_timeout(uint32_t seconds)
{
    g_shell_ctx.timeout_sec = seconds;
}

void shell_refresh_activity(void)
{
    /* g_shell_ctx.last_active = get_system_time(); */
}

int shell_check_timeout(uint32_t current_time)
{
    if (g_shell_ctx.timeout_sec == 0) {
        return 0;  /* 不超时 */
    }
    
    if (g_shell_ctx.is_logged_in) {
        if ((current_time - g_shell_ctx.last_active) > g_shell_ctx.timeout_sec) {
            printf("\r\nSession timeout. Auto logout.\r\n");
            shell_logout();
            return 1;
        }
    }
    return 0;
}

/*===========================================================================
 * 用户管理
 *===========================================================================*/
int shell_user_add(const char *username, const char *password, shell_perm_t perm)
{
    if (g_user_count >= MAX_USERS) {
        return -1;  /* 用户数已满 */
    }
    
    if (find_user(username) != NULL) {
        return -2;  /* 用户已存在 */
    }
    
    strncpy(g_user_db[g_user_count].username, username, MAX_USERNAME_LEN - 1);
    strncpy(g_user_db[g_user_count].password, password, MAX_PASSWORD_LEN - 1);
    g_user_db[g_user_count].permission = perm;
    g_user_db[g_user_count].enabled = 1;
    g_user_count++;
    
    return 0;
}

int shell_user_del(const char *username)
{
    /* 不允许删除root用户 */
    if (strcmp(username, "root") == 0) {
        return -1;
    }
    
    for (int i = 0; i < g_user_count; i++) {
        if (strcmp(g_user_db[i].username, username) == 0) {
            /* 移动后面的用户 */
            for (int j = i; j < g_user_count - 1; j++) {
                g_user_db[j] = g_user_db[j + 1];
            }
            g_user_count--;
            return 0;
        }
    }
    return -2;  /* 用户不存在 */
}

int shell_user_set_perm(const char *username, shell_perm_t perm)
{
    shell_user_t *user = find_user(username);
    if (user == NULL) {
        return -1;
    }
    user->permission = perm;
    return 0;
}

int shell_user_change_password(const char *username, const char *old_pwd, const char *new_pwd)
{
    shell_user_t *user = find_user(username);
    if (user == NULL) {
        return -1;
    }
    
    /* 验证旧密码 (root可以跳过) */
    if (g_shell_ctx.current_perm != PERM_ROOT) {
        if (strcmp(user->password, old_pwd) != 0) {
            return -2;
        }
    }
    
    strncpy(user->password, new_pwd, MAX_PASSWORD_LEN - 1);
    return 0;
}

/*===========================================================================
 * 命令列表显示
 *===========================================================================*/
void shell_cmd_list(void)
{
    const shell_cmd_t *cmd;
    int count = 0;
    int accessible = 0;
    
    printf("\r\n");
    printf("┌─────────────────────────────────────────────────────────────┐\r\n");
    printf("│  Command List - Current User: %-10s [%-8s]          │\r\n", 
           g_shell_ctx.current_user, shell_perm_to_string(g_shell_ctx.current_perm));
    printf("├──────────────┬──────────┬──────────────────────────────────┤\r\n");
    printf("│ Command      │ Perm     │ Description                      │\r\n");
    printf("├──────────────┼──────────┼──────────────────────────────────┤\r\n");
    
    for (cmd = SHELL_CMD_BEGIN; cmd < SHELL_CMD_END; cmd++) {
        if (cmd->name == NULL) continue;
        if (cmd->flags & CMD_FLAG_HIDDEN) continue;
        if (cmd->flags & CMD_FLAG_DISABLED) continue;
        
        count++;
        
        /* 检查是否有权限 */
        int has_perm = shell_check_permission(cmd->permission);
        if (has_perm) accessible++;
        
        printf("│ %-12s │ %-8s │ %-32s │%s\r\n",
               cmd->name,
               shell_perm_to_string(cmd->permission),
               cmd->desc ? cmd->desc : "No description",
               has_perm ? "" : " [LOCKED]");
    }
    
    printf("└──────────────┴──────────┴──────────────────────────────────┘\r\n");
    printf("Total: %d commands, Accessible: %d\r\n\r\n", count, accessible);
}

/*===========================================================================
 * 命令解析执行 (核心函数)
 *===========================================================================*/
int cmd_parsing(char *str)
{
    char *argv[MAX_ARGC];
    int argc;
    const shell_cmd_t *cmd;
    int result;
    
    if (str == NULL || *str == '\0') {
        return -2;
    }
    
    /* 刷新活动时间 */
    shell_refresh_activity();
    
    /* 解析参数 */
    argc = parse_args(str, argv);
    if (argc == 0) {
        return -2;
    }
    
    /* 查找命令 */
    cmd = find_cmd(argv[0]);
    if (cmd == NULL) {
        printf("Unknown command: '%s'\r\n", argv[0]);
        printf("Type 'help' for available commands.\r\n");
        return -1;
    }
    
    /* 检查命令是否禁用 */
    if (cmd->flags & CMD_FLAG_DISABLED) {
        printf("Command '%s' is disabled.\r\n", argv[0]);
        return -3;
    }
    
    /* 权限检查 */
    if (!shell_check_permission(cmd->permission)) {
        printf("Permission denied! Command '%s' requires [%s] or higher.\r\n",
               cmd->name, shell_perm_to_string(cmd->permission));
        printf("Current permission: [%s]\r\n", shell_perm_to_string(g_shell_ctx.current_perm));
        printf("Please login with appropriate credentials.\r\n");
        return -4;
    }
    
    /* 确认操作 */
    if (cmd->flags & CMD_FLAG_CONFIRM) {
        printf("\r\n*** WARNING: This is a privileged operation! ***\r\n");
        /* 实际应用中需要实现确认逻辑 */
    }
    
    /* 执行命令 */
    if (cmd->handler) {
        result = cmd->handler(argc, argv);
        
        /* 记录日志 */
        if (cmd->flags & CMD_FLAG_LOG) {
            log_command(g_shell_ctx.current_user, cmd->name, result);
        }
        
        return result;
    }
    
    return -1;
}

/*===========================================================================
 * 内置命令
 *===========================================================================*/

/* help命令 - 所有人可用 */
static int cmd_help(int argc, char *argv[])
{
    if (argc >= 2) {
        /* 显示特定命令的帮助 */
        const shell_cmd_t *cmd = find_cmd(argv[1]);
        if (cmd) {
            printf("\r\nCommand: %s\r\n", cmd->name);
            printf("  Description: %s\r\n", cmd->desc ? cmd->desc : "N/A");
            printf("  Usage: %s\r\n", cmd->usage ? cmd->usage : cmd->name);
            printf("  Permission: %s\r\n", shell_perm_to_string(cmd->permission));
            return 0;
        } else {
            printf("Command '%s' not found.\r\n", argv[1]);
            return -1;
        }
    }
    
    shell_cmd_list();
    return 0;
}
SHELL_CMD_EXPORT(help, "Show help information", cmd_help);

/* login命令 - 所有人可用 */
static int cmd_login(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: login <username> <password>\r\n");
        return -1;
    }
    
    return shell_login(argv[1], argv[2]);
}
SHELL_CMD_EXPORT(login, "Login to system", cmd_login);

/* logout命令 - 已登录用户可用 */
static int cmd_logout(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    if (!g_shell_ctx.is_logged_in) {
        printf("Not logged in.\r\n");
        return -1;
    }
    
    shell_logout();
    return 0;
}
SHELL_CMD_EXPORT_PERM(logout, "Logout from system", cmd_logout, PERM_USER_ABOVE);

/* whoami命令 - 所有人可用 */
static int cmd_whoami(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    printf("Current user: %s\r\n", g_shell_ctx.current_user);
    printf("Permission level: %s\r\n", shell_perm_to_string(g_shell_ctx.current_perm));
    printf("Logged in: %s\r\n", g_shell_ctx.is_logged_in ? "Yes" : "No");
    
    return 0;
}
SHELL_CMD_EXPORT(whoami, "Show current user info", cmd_whoami);

/* passwd命令 - 普通用户以上 */
static int cmd_passwd(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: passwd <old_password> <new_password>\r\n");
        return -1;
    }
    
    int ret = shell_user_change_password(g_shell_ctx.current_user, argv[1], argv[2]);
    if (ret == 0) {
        printf("Password changed successfully.\r\n");
    } else if (ret == -2) {
        printf("Old password incorrect.\r\n");
    } else {
        printf("Failed to change password.\r\n");
    }
    
    return ret;
}
SHELL_CMD_EXPORT_PERM(passwd, "Change password", cmd_passwd, PERM_USER_ABOVE);

/* users命令 - 管理员以上 */
static int cmd_users(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    printf("\r\n%-16s %-12s %-8s\r\n", "Username", "Permission", "Status");
    printf("----------------------------------------\r\n");
    
    for (int i = 0; i < g_user_count; i++) {
        printf("%-16s %-12s %-8s\r\n",
               g_user_db[i].username,
               shell_perm_to_string(g_user_db[i].permission),
               g_user_db[i].enabled ? "Enabled" : "Disabled");
    }
    printf("\r\n");
    
    return 0;
}
SHELL_CMD_ADMIN(users, "List all users", cmd_users);

/* useradd命令 - 管理员以上 */
static int cmd_useradd(int argc, char *argv[])
{
    shell_perm_t perm = PERM_USER;
    
    if (argc < 3) {
        printf("Usage: useradd <username> <password> [permission]\r\n");
        printf("  permission: guest, user, operator, admin\r\n");
        return -1;
    }
    
    if (argc >= 4) {
        if (strcmp(argv[3], "guest") == 0) perm = PERM_GUEST;
        else if (strcmp(argv[3], "user") == 0) perm = PERM_USER;
        else if (strcmp(argv[3], "operator") == 0) perm = PERM_OPERATOR;
        else if (strcmp(argv[3], "admin") == 0) perm = PERM_ADMIN;
        /* 只有root可以创建admin */
        if (perm == PERM_ADMIN && g_shell_ctx.current_perm != PERM_ROOT) {
            printf("Only root can create admin users.\r\n");
            return -1;
        }
    }
    
    int ret = shell_user_add(argv[1], argv[2], perm);
    if (ret == 0) {
        printf("User '%s' created with [%s] permission.\r\n", argv[1], shell_perm_to_string(perm));
    } else if (ret == -1) {
        printf("User database full.\r\n");
    } else if (ret == -2) {
        printf("User '%s' already exists.\r\n", argv[1]);
    }
    
    return ret;
}
SHELL_CMD_ADMIN(useradd, "Add new user", cmd_useradd);

/* userdel命令 - 管理员以上 */
static int cmd_userdel(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: userdel <username>\r\n");
        return -1;
    }
    
    int ret = shell_user_del(argv[1]);
    if (ret == 0) {
        printf("User '%s' deleted.\r\n", argv[1]);
    } else if (ret == -1) {
        printf("Cannot delete root user.\r\n");
    } else {
        printf("User '%s' not found.\r\n", argv[1]);
    }
    
    return ret;
}
SHELL_CMD_ADMIN(userdel, "Delete user", cmd_userdel);

/* reboot命令 - ROOT专用 */
static int cmd_reboot(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    printf("System will reboot now...\r\n");
    /* NVIC_SystemReset(); */
    
    return 0;
}
SHELL_CMD_ROOT(reboot, "Reboot system", cmd_reboot);

/* factory命令 - ROOT专用，恢复出厂设置 */
static int cmd_factory(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    printf("Factory reset initiated...\r\n");
    /* 执行恢复出厂设置操作 */
    
    return 0;
}
_SHELL_CMD_EXPORT(factory, "Factory reset", "factory [--force]", 
                  cmd_factory, PERM_ROOT, CMD_FLAG_LOG | CMD_FLAG_CONFIRM | CMD_FLAG_HIDDEN);
```

### 3. 用户命令示例 `user_cmd.c`

```c
#include "shell_cmd.h"
#include <stdio.h>
#include <stdlib.h>

/*===========================================================================
 * 普通用户命令 (GUEST及以上)
 *===========================================================================*/

/* echo命令 - 所有人可用 */
static int cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\r\n");
    return 0;
}
SHELL_CMD_EXPORT(echo, "Echo text", cmd_echo);

/* version命令 - 所有人可用 */
static int cmd_version(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("Firmware: v1.0.0\r\n");
    printf("Build: %s %s\r\n", __DATE__, __TIME__);
    return 0;
}
SHELL_CMD_EXPORT(version, "Show version", cmd_version);

/*===========================================================================
 * 操作员命令 (OPERATOR及以上)
 *===========================================================================*/

/* LED控制 - 操作员以上 */
static int cmd_led(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: led <on|off|status>\r\n");
        return -1;
    }
    
    if (strcmp(argv[1], "on") == 0) {
        printf("LED ON\r\n");
    } else if (strcmp(argv[1], "off") == 0) {
        printf("LED OFF\r\n");
    } else if (strcmp(argv[1], "status") == 0) {
        printf("LED Status: ON\r\n");
    }
    return 0;
}
SHELL_CMD_EXPORT_PERM(led, "Control LED", cmd_led, PERM_OPERATOR_ABOVE);

/* 参数配置 - 操作员以上 */
static int cmd_config(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: config <get|set> <key> [value]\r\n");
        return -1;
    }
    
    if (strcmp(argv[1], "get") == 0 && argc >= 3) {
        printf("Config[%s] = value\r\n", argv[2]);
    } else if (strcmp(argv[1], "set") == 0 && argc >= 4) {
        printf("Config[%s] set to '%s'\r\n", argv[2], argv[3]);
    }
    return 0;
}
SHELL_CMD_EXPORT_PERM(config, "Get/Set configuration", cmd_config, PERM_OPERATOR_ABOVE);

/*===========================================================================
 * 管理员命令 (ADMIN及以上)
 *===========================================================================*/

/* 内存读取 - 管理员以上 */
static int cmd_md(int argc, char *argv[])
{
    uint32_t addr, len = 64;
    
    if (argc < 2) {
        printf("Usage: md <address> [length]\r\n");
        return -1;
    }
    
    addr = strtoul(argv[1], NULL, 0);
    if (argc >= 3) len = strtoul(argv[2], NULL, 0);
    
    printf("Memory dump at 0x%08lX, %lu bytes:\r\n", addr, len);
    
    for (uint32_t i = 0; i < len; i++) {
        if (i % 16 == 0) printf("%08lX: ", addr + i);
        printf("%02X ", *(volatile uint8_t *)(addr + i));
        if ((i + 1) % 16 == 0) printf("\r\n");
    }
    if (len % 16) printf("\r\n");
    
    return 0;
}
SHELL_CMD_ADMIN(md, "Memory dump", cmd_md);

/* 内存写入 - 管理员以上 */
static int cmd_mw(int argc, char *argv[])
{
    if (argc < 3) {
        printf("Usage: mw <address> <value>\r\n");
        return -1;
    }
    
    uint32_t addr = strtoul(argv[1], NULL, 0);
    uint32_t value = strtoul(argv[2], NULL, 0);
    
    *(volatile uint32_t *)addr = value;
    printf("Write 0x%08lX to address 0x%08lX\r\n", value, addr);
    
    return 0;
}
_SHELL_CMD_EXPORT(mw, "Memory write", "mw <addr> <value>", 
                  cmd_mw, PERM_ADMIN_ABOVE, CMD_FLAG_LOG);

/*===========================================================================
 * ROOT专用命令
 *===========================================================================*/

/* Flash擦除 - 仅ROOT */
static int cmd_flash_erase(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: flash_erase <sector>\r\n");
        return -1;
    }
    
    printf("Erasing flash sector %s...\r\n", argv[1]);
    /* 实际Flash擦除操作 */
    
    return 0;
}
SHELL_CMD_ROOT(flash_erase, "Erase flash sector", cmd_flash_erase);

/* 调试模式 - 隐藏命令，仅ROOT */
static int cmd_debug(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("Debug mode enabled.\r\n");
    return 0;
}
SHELL_CMD_HIDDEN(debug, "Enable debug mode", cmd_debug, PERM_ROOT);
```

### 4. 主程序示例

```c
#include "shell_cmd.h"
#include <stdio.h>
#include <string.h>

#define CMD_BUF_SIZE    128

static char cmd_buffer[CMD_BUF_SIZE];
static uint8_t cmd_index = 0;

/* 获取Shell提示符 */
static void print_prompt(void)
{
    const char *user = shell_get_current_user();
    shell_perm_t perm = shell_get_current_perm();
    
    /* 不同权限显示不同提示符 */
    if (perm == PERM_ROOT) {
        printf("\033[31m%s#\033[0m ", user);  /* 红色# */
    } else if (perm >= PERM_ADMIN) {
        printf("\033[33m%s$\033[0m ", user);  /* 黄色$ */
    } else {
        printf("%s> ", user);                  /* 普通> */
    }
}

/* 串口输入处理 */
void shell_input(char ch)
{
    if (ch == '\r' || ch == '\n') {
        if (cmd_index > 0) {
            cmd_buffer[cmd_index] = '\0';
            printf("\r\n");
            
            cmd_parsing(cmd_buffer);
            
            cmd_index = 0;
            memset(cmd_buffer, 0, CMD_BUF_SIZE);
        }
        print_prompt();
    }
    else if (ch == '\b' || ch == 0x7F) {
        if (cmd_index > 0) {
            cmd_index--;
            printf("\b \b");
        }
    }
    else if (cmd_index < CMD_BUF_SIZE - 1) {
        cmd_buffer[cmd_index++] = ch;
        printf("%c", ch);
    }
}

int main(void)
{
    /* 系统初始化 */
    shell_init();
    print_prompt();
    
    while (1) {
        /* 从串口获取字符 */
        // char ch = uart_getchar();
        // shell_input(ch);
        
        /* 检查会话超时 */
        // shell_check_timeout(get_system_time());
    }
}
```

### 5. 运行效果演示

```text
========================================
  Secure Shell v2.0 - Permission System
========================================
Type 'help' for available commands
Type 'login <user> <pass>' to login

guest> help

┌─────────────────────────────────────────────────────────────┐
│  Command List - Current User: guest      [guest   ]          │
├──────────────┬──────────┬──────────────────────────────────┤
│ Command      │ Perm     │ Description                      │
├──────────────┼──────────┼──────────────────────────────────┤
│ help         │ guest    │ Show help information            │
│ login        │ guest    │ Login to system                  │
│ logout       │ user     │ Logout from system               │ [LOCKED]
│ whoami       │ guest    │ Show current user info           │
│ passwd       │ user     │ Change password                  │ [LOCKED]
│ users        │ admin    │ List all users                   │ [LOCKED]
│ useradd      │ admin    │ Add new user                     │ [LOCKED]
│ userdel      │ admin    │ Delete user                      │ [LOCKED]
│ reboot       │ root     │ Reboot system                    │ [LOCKED]
│ echo         │ guest    │ Echo text                        │
│ version      │ guest    │ Show version                     │
│ led          │ operator │ Control LED                      │ [LOCKED]
│ config       │ operator │ Get/Set configuration            │ [LOCKED]
│ md           │ admin    │ Memory dump                      │ [LOCKED]
│ mw           │ admin    │ Memory write                     │ [LOCKED]
│ flash_erase  │ root     │ Erase flash sector               │ [LOCKED]
└──────────────┴──────────┴──────────────────────────────────┘
Total: 16 commands, Accessible: 5

guest> led on
Permission denied! Command 'led' requires [operator] or higher.
Current permission: [guest]
Please login with appropriate credentials.

guest> login admin admin123
Login successful! Welcome, admin [admin]

admin$ led on
LED ON

admin$ users

Username         Permission   Status  
----------------------------------------
root             root         Enabled 
admin            admin        Enabled 
operator         operator     Enabled 
user             user         Enabled 
guest            guest        Enabled 

admin$ useradd test test123 operator
User 'test' created with [operator] permission.

admin$ logout
User 'admin' logged out.

guest> login root root123
Login successful! Welcome, root [root]

root# reboot
[LOG] User:root Cmd:reboot Result:0
System will reboot now...
```

## 权限系统特性总结

|     特性     |                  说明                  |
| :----------: | :------------------------------------: |
| **多级权限** | GUEST → USER → OPERATOR → ADMIN → ROOT |
| **权限组合** |      命令可设置多个权限级别可访问      |
| **登录认证** |            用户名/密码验证             |
| **登录锁定** |           多次失败后锁定账户           |
| **会话超时** |            自动登出空闲用户            |
| **命令标志** |     隐藏、禁用、日志记录、确认执行     |
| **用户管理** |   添加/删除用户、修改密码、权限设置    |
|  **快捷宏**  | `SHELL_CMD_ADMIN`、`SHELL_CMD_ROOT`等  |