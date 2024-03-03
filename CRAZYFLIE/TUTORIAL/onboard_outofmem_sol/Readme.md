**ps. 這是一個針對crazyflie燒錄img files時因記憶體不足無法燒錄的解決方法**

# Onboard out of memory solve

## 1. Reset devices(update firmware to free malloc)
### gap_sdk
OS Requirements installation
```bash
sudo apt-get install -y \
    autoconf \
    automake \
    bison \
    build-essential \
    cmake \
    curl \
    doxygen \
    flex \
    git \
    gtkwave \
    libftdi-dev \
    libftdi1 \
    libjpeg-dev \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libsndfile1-dev \
    graphicsmagick-libmagick-dev-compat \
    libtool \
    libusb-1.0-0-dev \
    pkg-config \
    python3-pip \
    rsync \
    scons \
    texinfo \
    wget
```

Python Package Management
```bash
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 10
```

Download and install the toolchain
```bash
git clone https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu.git
```

Install the toolchain (this may require to launch the script through sudo):
```bash
cd gap_riscv_toolchain_ubuntu
./install.sh
```

> [!TIP]
> You can go to previous folder cd..

Clone the actual gap_sdk repository
```bash
git clone https://github.com/GreenWaves-Technologies/gap_sdk.git
```

Configure the SDK and go into the update firmware.c
```bash
cd gap_sdk/
source configs/ai_deck.sh
```

Our modules (gapy runner) require a few additional Python packages that you can install with this command from GAP SDK root folder:
```bash
pip3 install -r requirements.txt
pip3 install -r doc/requirements.txt
```

Once in the SDK, run ``make help`` to get commands and get SDK ready to use.
```bash
$ make help
=================== GAP SDK ===================

Main targets:
 - clean       : clean the SDK
 - all         : build the whole SDK with all tools
 - minimal     : get latest sources for all rtos and libs
 - gvsoc       : build GVSOC simulation platform
 - openocd.all : build OpenOCD tools to run simulation on boards
 - nntool      : build nntool
```

Then, depends on what you need, build the SDK accordingly (suggest use all)
```bash
make all
```

*Finally, go into the folder where yje update_firmware.c code is and run it*
```bash
cd gap_sdk/examples/gap8/utils/firmware_update/simple
make clean all run PMSIS_OS=freertos platform=board
```
#
#
#
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
