编译前需要定义gcc路径，DAP下载需要pyOCD。当前Makefile默认使用 D:\ETools\pyocd\pyocd.exe，JLink仍保留为可选目标。
1：在make命令后加 GCC_PATH/JLINK_PATH变量，如：
make GCC_PATH="D:\\Program Files (x86)\\GNU Arm Embedded Toolchain\\10 2020-q4-major\\bin\\"
make download-jlink JLINK_PATH="D:\\Program Files (x86)\\SEGGER\\JLink_V632g\\"
2、将GCC_PATH/JLINK_PATH添加到电脑的环境变量
3、在makefile里直接添加

以下命令假设以方法2/3定义GCC_PATH/JLINK_PATH。
一、编译
      make :编译,带debug信息；
      make release=1:编译,不带debug信息；
      
 二、DAP下载
      make flash:使用DAP/pyOCD下载；
      make download:同make flash；
      默认目标为n32l406cb，默认CMSIS-Pack为Nationstech.N32L40x_DFP.1.4.0.pack。
      如需覆盖：make flash PYOCD="D:/ETools/pyocd/pyocd.exe" PYOCD_TARGET=n32l406cb PYOCD_FREQ=4000000

 三、JLink下载/调试
      make download-jlink:使用JLink下载；
      make debug-jlink:使用JLink调试；

 四、调试
      DAP调试建议由编辑器/pyOCD GDB Server配置，工程Makefile当前只保证DAP烧录路径。
