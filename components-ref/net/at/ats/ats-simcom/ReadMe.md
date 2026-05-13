## AT 命令概述

### 惯例和术语缩写
本手册中模块可以被称为如下术语：
1）ME (移动设备)，它可包括移动终端（MT），终端适配器（TA）
2）MS (移动台)，它包括移动设备（ME）和用户识别模块（SIM）
3）TA (终端设配器)
4）DCE (数据通信设备) 或者传真 DCE (传真调制解调器，传真板)

## TBD
- 设置自动追加命令


### AT 命令语法
本手册中所有命令行必须以"AT"或"at"作为开头，以回车（<CR>）作为结尾。响应通常紧随命令之后，它的样式是“<
回车><新行><响应内容><回车><新行>”(<CR><LF><响应内容><CR><LF>) 。整本手册里，只有<响应内容>
被自始至终介绍，而<回车><新行>被有意省略了。
合宙无线模块提供的AT 命令包含符合GSM07.05、GSM07.07 和ITU-T Recommendation V.25ter 的命令.
所有AT 命令从语法上可以分为三类：“基础类”，“S 参数类”以及“扩展类”。


## AT 透传切换设计(Quectel)
1. 为防止+++转义序列被误解为数据，应遵循以下规则：
1.1 输入+++前 1 秒内请勿输入任何字符。
1.2 1秒钟内输入+++，在此时间内不能输入其他字符。
1.3 输入+++后 1 秒钟内请勿输入任何字符。
1.4 成功切换到命令模式；否则，返回步骤 1.1。
2. 如需从命令模式切换至数据模式，请输入 ATO。
3. 切换为命令模式的另一种方法是通过更改 DTR 电平，有关详细信息，参见 AT&D。


## AT 透传切换涉及（Air）
说明：为避免+++ 被错误的识别为数据，需要遵循以下步骤：
1. “+++”输入前 T1 时间（1 秒）内无字符输入。
2. 在 0.5 second 内连续输入三个+号，每个+号之间不能有其他字符。
3. “+++”输入后 T1 时间（0.5 秒）内无字符输入。
4.切换至命令模式，否则重新进入步骤 1


## 错误响应
- strstr(strResponse, "+CME ERROR:") // +CME ERROR Command from equipment
- strstr(strResponse, "+CMS ERROR:") // +CMS ERROR Command from Server
- strstr(strResponse, "ERROR")     // Unknown ERROR

## 关于内部AT发送函数
- 虚拟一个port？？
- 发送消息队列，
- 如果有接收缓存则放到缓存，否则则放到uart口？也就是最终会判断发送到哪个口了？？
  - 各种接口调用：msg_send
    - port 口


## ERROR List
- CME ERROR
- CMS ERROR
- xx ERROR
- ERROR （unknown error）

### CME, CMS Error
- https://www.cnblogs.com/listenerln/p/7272850.html
- https://blog.csdn.net/u014436243/article/details/103008767