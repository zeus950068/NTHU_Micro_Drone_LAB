**ps. 這是一個在linux上安裝crazyflie aideck的教學**

# Installation of Crazyflie aideck
## 1. Prerequisites
For < Ubuntu 20.04 you will need to check first if which version your python is on and if you have 'python3' on your system.
From a fresh Ubuntu 20.04 system and up, running the client form source requires git, pip and a lib for the Qt GUI.
```bash
sudo apt install git python3-pip libxcb-xinerama0 libxcb-cursor0
pip3 install --upgrade pip
```


## 2. install cfclient from latest release
Each release of the client is pushed to the [pypi repository](https://pypi.org/), so it can be installed with pip:
```bash
pip3 install cfclient
```

Clone the repository with git
```bash
git clone https://github.com/bitcraze/crazyflie-clients-python
cd crazyflie-clients-python
```

### Installing the client
All other dependencies on linux are handled by pip so to install an editable copy simply run:
```bash
pip3 install -e .
```


## 3. Update Crazyflie and AIdeck firmware (USING CFCLIENT)
> [!NOTE]
> Make sure that only the AI-deck is attached to the Crazyflie, with no other deck.

1. Open up the cfclient on your computer
2. Make sure that only the AI-deck is attached to the Crazyflie, with no other deck.
3. Go to ‘Connect’->’bootloader’
4. Type the address of your crazyflie, press ‘Scan’ and select your crazyflie’s URI. Make sure to choose ‘radio://…’ (not ‘usb://’). Now press ‘Connect’
5. In the ‘Firmware Source’ section, select the latest release in ‘Available downloads’. Make sure to select the right platform (cf2 is for the crazyflie 2.x ).
6. Press ‘Program’ and wait for the STM, NRF and ESP MCUs to be re-flashed. The crazyflie will restart a couple of times, and the flashing of the ESP (‘bcAI:esp deck memory’) takes about 3 minutes.
7. Once the status states ‘Idle’ and the Crazyflie is disconnected, double check if the flashing has succeded. In the cfclient, connect to the crazyflie and check in the console tab if you see: ESP32: I (910) SYS: Initialized. Also LED1 should be flashing with 2 hz.

step1: 

![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/install%20aideck/build%20firmware(1).png)

step2:

![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/install%20aideck/build%20firmware(2).png)


## 4. Gap8 bootloader
> You will need to flash the bootloader on the GAP8 separately. This can only be done from a native linux computer or virtual machine (not WSL) with a jtag enabled programmer (Olimex ARM-USB-TINY-H JTAG or Jlink).

:+1:You only need to do this once and then you can enjoy the benefits of over-the-air flashing.

Clone, build and flash the bootloader with an Olimex ARM-USB-TINY-H JTAG or a Jlink using the following commands:
```bash
git clone https://github.com/bitcraze/aideck-gap8-bootloader.git
cd aideck-gap8-bootloader
docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/;  make all image flash'
```

Once you see the following it means you were successful
```bash
--------------------------
flasher is done!
--------------------------
--------------------------
Reset CONFREG to 0
--------------------------
GAP8 examine target
RESET: jtag boot mode=3
DEPRECATED! use 'adapter [de]assert' not 'jtag_reset'
```
