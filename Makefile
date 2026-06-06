CC ?= gcc

define CLANGD_FLAGS
-std=c99
-DPC_BUILD=1
-DXY_LOG_LEVEL=3
-DAT_MEM_WATCH_EN=0
-DAT_MEM_LIMIT_SIZE=65536
-DN32L40X
-DUSE_STDPERIPH_DRIVER
-Icomponents/actuator
-Icomponents/ais
-Icomponents/at/include
-Icomponents/ats
-Icomponents/btn
-Icomponents/cell
-Icomponents/clib
-Icomponents/cmd
-Icomponents/crypto/inc
-Icomponents/dsc
-Icomponents/epirb
-Icomponents/event
-Icomponents/gnss
-Icomponents/gui
-Icomponents/io
-Icomponents/json
-Icomponents/log
-Icomponents/mem
-Icomponents/modbus
-Icomponents/norflash
-Icomponents/pt
-Icomponents/sensor/core
-Icomponents/storage/eeprom
-Icomponents/storage/evtlog
-Icomponents/storage/fee
-Icomponents/storage/flog
-Icomponents/storage/param
-Icomponents/storage/vlog
-Icomponents/sys
-Icomponents/timer
-Icomponents/tlv
-Icomponents/uwb
-Icomponents/wifi
-Ihal/Hal-N32L40x/CMSIS/core
-Ihal/Hal-N32L40x/CMSIS/device
-Ihal/Hal-N32L40x/n32l40x_std_periph_driver/inc
-Ihal/Hal-N32L40x/n32l40x_usbfs_driver/inc
-Iproject/PLB -N32/USER/inc
-Iproject/Tiny-App/src
-Iproject/Tiny-App/tests
-Iproject/plb/src
-Iproject/plb/tests
endef

.PHONY: zed clangd-flags clean-zed

zed: clangd-flags

clangd-flags:
	$(file >compile_flags.txt,$(CLANGD_FLAGS))
	@echo wrote compile_flags.txt

clean-zed:
	$(RM) compile_flags.txt
