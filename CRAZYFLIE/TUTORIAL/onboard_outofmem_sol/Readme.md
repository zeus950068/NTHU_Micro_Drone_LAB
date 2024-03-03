**ps. 這是一個針對crazyflie燒錄img files時因記憶體不足無法燒錄的解決方法**

# Onboard out of memory solve

## 1. Reset devices(update firmware to free malloc)
### gap_sdk
下載gap_sdk
```bash
git clone https://github.com/GreenWaves-Technologies/gap_sdk.git
```
```bash
cd gap_sdk/
source configs/ai_deck.sh
cd gap_sdk/examples/gap8/utils/firmware_update/simple
```

### gap8_openocd
下載gap8_openocd
```bash
git clone https://github.com/GreenWaves-Technologies/gap8_openocd.git
```
```bash
export GAPY_OPENOCD_CABLE=~/Desktop/<path/to/gap8_openocd>/gap8_openocd/tcl/interface/ftdi/olimex-arm-usb-tiny-h.cfg
make clean all run PMSIS_OS=freertos platform=board
```

## 2. let cfloader run 0% to 100%
### aideck-gap8-bootloader
下載 aideck-gap8-bootloader
```bash
git clone https://github.com/bitcraze/aideck-gap8-bootloader.git
```
```bash
cd aideck-gap8-bootloader/
docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/;  make all image flash'
```
