**ps. 這是一個針對crazyflie燒錄img files時因記憶體不足無法燒錄的解決方法**

# Onboard out of memory solve

## Reset devices(update firmware to free malloc)
### 1. gap_sdk
下載gap_sdk
```bash
(1)	git clone https://github.com/GreenWaves-Technologies/gap_sdk.git
(2)	cd gap_sdk/
(3)	source configs/ai_deck.sh
(4)	cd gap_sdk/examples/gap8/utils/firmware_update/simple
```

### 2. gap8_openocd
下載gap8_openocd
```bash
(1)	git clone https://github.com/GreenWaves-Technologies/gap8_openocd.git
(2)	export GAPY_OPENOCD_CABLE=~/Desktop/<path/to/gap8_openocd>/gap8_openocd/tcl/interface/ftdi/olimex-arm-usb-tiny-h.cfg
(3)	make clean all run PMSIS_OS=freertos platform=board
```

## let cfloader run 0% to 100%
### 3. aideck-gap8-bootloader
下載 aideck-gap8-bootloader
```bash
(1)	git clone https://github.com/bitcraze/aideck-gap8-bootloader.git
(2)	cd aideck-gap8-bootloader/
(3)	docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/;  make all image flash'
```
