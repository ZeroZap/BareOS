######################################
# target
######################################
TARGET = PLB

######################################
# building variables
######################################
# debug build
ifeq ($(release), y)
DEBUG = 0
else
DEBUG = 1
endif
# optimization
OPT = -O0

#######################################
# Build path
#######################################
BUILD_DIR = build

#######################################
# HAL path
#######################################
HAL_DIR ?= ../../../hal/Hal-N32L40x
COMPONENTS_DIR ?= ../../../components

#####################################
# chip platform info
######################################
TARGET_STARTUP  := n32l40x
## NTFx CODE START Define ##
DEFS += -DN32L40X
DEFS += -DUSE_STDPERIPH_DRIVER
## NTFx CODE END Define ##
######################################
# Algo libs
######################################
#USELIB = 0

######################################
#TOOLS CHAIN
######################################
# The gcc path can be defined by 3 methods:
# 1.Define it in make command via GCC_PATH variable (> make GCC_PATH="D:\\Program Files (x86)\\GNU Arm Embedded Toolchain\\10 2020-q4-major\\bin\\")
# 2.Added it to the PATH environment variable
# 3.Define it in makefile ,such as :
#GCC_PATH = 'D:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2020-q4-major\bin\'
ifdef GCC_PATH
CROSS_COMPILE = $(GCC_PATH)arm-none-eabi-
else 
CROSS_COMPILE = arm-none-eabi-
endif


######################################
# C sources
######################################
## NTFx CODE START C sources ##
C_DIRS += ../USER/src
C_DIRS += $(HAL_DIR)/CMSIS/core
C_DIRS += $(HAL_DIR)/CMSIS/device
C_DIRS += $(HAL_DIR)/n32l40x_std_periph_driver/src
C_DIRS += $(HAL_DIR)/n32l40x_usbfs_driver/src
C_DIRS += $(COMPONENTS_DIR)/clib
C_DIRS += $(COMPONENTS_DIR)/log
## NTFx CODE END C sources ##

SRC_OBJS_DIRS += $(foreach DIR, $(C_DIRS), $(wildcard $(DIR)/*.c))
C_SOURCES = $(SRC_OBJS_DIRS)

######################################
# ASM sources
######################################
ASM_SOURCES =$(HAL_DIR)/CMSIS/device/startup/startup_$(TARGET_STARTUP)_gcc.s

######################################
# C includes
######################################
## NTFx CODE START C includes ##
C_INCS += ../USER/inc
C_INCS += $(HAL_DIR)/CMSIS/core
C_INCS += $(HAL_DIR)/CMSIS/device
C_INCS += $(HAL_DIR)/n32l40x_std_periph_driver/inc
C_INCS += $(HAL_DIR)/n32l40x_usbfs_driver/inc
C_INCS += $(COMPONENTS_DIR)/clib
C_INCS += $(COMPONENTS_DIR)/log
C_INCS += $(COMPONENTS_DIR)/mem
## NTFx CODE END C includes ##

INCS_OBJS_DIR = $(foreach DIR2, $(C_INCS), $(wildcard $(DIR2)/*.h))
INCS_OBJS_PATH = $(sort $(dir $(INCS_OBJS_DIR)))
C_INCLUDES = $(addprefix -I,$(INCS_OBJS_PATH))

######################################
# Lib files
######################################
ifeq ($(USELIB), 1)
SRC_LIB_DIR += $(HAL_DIR)/n32l40x_algo_lib/lib
INC_LIB_DIR += $(HAL_DIR)/n32l40x_algo_lib/inc
LIB_SOURCES = $(foreach DIR3, $(SRC_LIB_DIR), $(wildcard $(DIR3)/*.lib))
LIB_SOURCES_L = $(addprefix -L,$(LIB_SOURCES))
C_LIBS = $(LIB_SOURCES_L)
LIB_INCS = $(foreach DIR4, $(INC_LIB_DIR), $(wildcard $(DIR4)/*.h))
LIB_INCS_PATH = $(sort $(dir $(LIB_INCS)))
LIB_INCS_PATH_I = $(addprefix -I,$(LIB_INCS_PATH))
C_INCLUDES += $(LIB_INCS_PATH_I)
endif

######################################
# Compile & Link flags
######################################
# cpu
CPU = -mcpu=cortex-m4
# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

#CFLAGS
CFLAGS += $(MCU) -Wall
CFLAGS += $(OPT)
CFLAGS += -ffunction-sections -fdata-sections
#DEBUG
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
#LFLAGS
LFLAGS += $(MCU)
LFLAGS += -Wl,--gc-sections 
LFLAGS += --specs=nosys.specs
LFLAGS += -Xlinker -Map=$(BUILD_DIR)/$(TARGET).map
# link script
LDSCRIPT =PLB.ld

######################################
# Objects
######################################
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	"$(CROSS_COMPILE)gcc" $(CFLAGS) $(DEFS) $(C_INCLUDES) $(C_LIBS) -c -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
    
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	"$(CROSS_COMPILE)gcc" $(CFLAGS) -c $< -o $@
    
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	"$(CROSS_COMPILE)gcc" $(OBJECTS) $(LFLAGS) -T$(LDSCRIPT) -o $@
	"$(CROSS_COMPILE)size" $@
	
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	"$(CROSS_COMPILE)objcopy" -O binary -S $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	"$(CROSS_COMPILE)objcopy" -O ihex -S $< $@
    
$(BUILD_DIR):
	@-mkdir $@

SHELL=cmd.exe
#######################################
# clean up
#######################################
clean:
	@if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
#	@-rm -rf $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

#######################################
# flash .hex/.bin by DAP pyOCD
#######################################
PYOCD ?= D:/ETools/pyocd/pyocd.exe
PYOCD_PACK ?= Nationstech.N32L40x_DFP.1.4.0.pack
PYOCD_TARGET ?= n32l406cb
PYOCD_FREQ ?= 400000

flash: all
	@if not exist "$(PYOCD)" (echo pyOCD not found: $(PYOCD) && echo Install pyOCD or run: make flash PYOCD=path/to/pyocd.exe && exit /b 1)
	@"$(PYOCD)" load --pack "$(PYOCD_PACK)" -t $(PYOCD_TARGET) -f $(PYOCD_FREQ) --erase sector $(BUILD_DIR)/$(TARGET).hex
	@echo "Flash Completed!"

download: flash

#######################################
# download .hex/.bin by jlink
#######################################
#Your JLink installation directory
# The jlink.exe path can be defined by 3 methods:
# 1.Define it in make command via JLINK_PATH variable (> make download-jlink JLINK_PATH="D:\\Program Files (x86)\\SEGGER\\JLink_V632g\\")
# 2.Added it to the PATH environment variable
# 3.Define it in makefile ,such as :
#JLINK_PATH = 'D:\Program Files (x86)\SEGGER\JLink_V632g\'
ifdef JLINK_PATH
JK_DPATH = $(JLINK_PATH)
else 
JK_DPATH=""
endif


#Chip type
CHIP_TYPE = N32L406CB

download-jlink:
	@"$(JK_DPATH)JLink.exe" -device $(CHIP_TYPE) -if SWD -speed 4000 -autoconnect 1 -CommanderScript PLB.jlink
	@echo "Download Completed!"

debug-jlink:
	@"$(JK_DPATH)JLinkGDBServer.exe" -select USB -device $(CHIP_TYPE) -if SWD -speed auto -noir -LocalhostOnly

# *** EOF ***
