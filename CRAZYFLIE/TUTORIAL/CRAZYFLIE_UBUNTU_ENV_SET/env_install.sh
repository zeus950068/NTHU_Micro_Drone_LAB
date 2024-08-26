#========Download Chrome========
cd Downloads
wget -c https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb
sudo dpkg -i google-chrome-stable_current_amd64.deb
sudo apt-get install -f
sudo apt-get remove chrome-chrome-stable


#========Nvidia driver for dual monitor========
sudo ubuntu-drivers list
sudo ubuntu-drivers install nvidia:535


#========USB Permission========
sudo groupadd plugdev
sudo usermod -a -G plugdev $USER

# Copy-paste the following in your console, this will create the file /etc/udev/rules.d/99-bitcraze.rules:
cat <<EOF | sudo tee /etc/udev/rules.d/99-bitcraze.rules > /dev/null
# Crazyradio (normal operation)
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="7777", MODE="0664", GROUP="plugdev"
# Bootloader
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="0101", MODE="0664", GROUP="plugdev"
# Crazyflie (over USB)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5740", MODE="0664", GROUP="plugdev"
EOF

# You can reload the udev-rules using the following:
sudo udevadm control --reload-rules
sudo udevadm trigger


#========Install Docker Engine========
# Add Docker's official GPG key:
sudo apt-get update
sudo apt-get install ca-certificates curl
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

# Add the repository to Apt sources:
echo \
  "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
  sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
sudo apt-get update

sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

sudo docker run hello-world


#========Setup development environment========
cd ~/Desktop
mkdir Greenwaves
cd Greenwaves

git clone https://github.com/GreenWaves-Technologies/gap8_openocd.git

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


git clone https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu.git
cd gap_riscv_toolchain_ubuntu
./install.sh

git clone https://github.com/GreenWaves-Technologies/gap_sdk.git
cd gap_sdk
source sourceme.sh
source configs/ai_deck.sh
pip3 install -r requirements.txt
pip3 install -r doc/requirements.txt
make clean
make sdk

# Setting up docker and the autotiler
sudo docker run --rm -it --name myAiDeckContainer bitcraze/aideck
# 開啟一個新的terminal(原本的先不要關）
sudo docker commit myAiDeckContainer aideck-with-autotiler
# 回到原本的termianl
exit
make autotiler

sudo cp ~/Desktop/Greenwaves/gap8_openocd/contrib/60-openocd.rules /etc/udev/rules.d
sudo udevadm control --reload-rules && sudo udevadm trigger

#sudo usermod -a -G dialout <username>
sudo usermod -a -G dialout user

cd examples/gap8/basic/helloworld
export GAPY_OPENOCD_CABLE=~/Desktop/Greenwaves/gap8_openocd/tcl/interface/ftdi/olimex-arm-usb-tiny-h.cfg
make clean all run PMSIS_OS=freertos platform=board

#========Install cfclient========
sudo apt install git python3-pip libxcb-xinerama0 libxcb-cursor0
pip3 install --upgrade pip
python3 -m pip install pip setuptools --upgrade
pip3 install cfclient

#========aideck-gap8-bootloader========
cd ~/Desktop
mkdir Bitcraze
cd Bitcraze
git clone https://github.com/bitcraze/aideck-gap8-bootloader.git
cd aideck-gap8-bootloader
sudo docker run --rm -it -v $PWD:/module/ --device /dev/ttyUSB0 --privileged -P bitcraze/aideck /bin/bash -c 'export GAPY_OPENOCD_CABLE=interface/ftdi/olimex-arm-usb-tiny-h.cfg; source /gap_sdk/configs/ai_deck.sh; cd /module/;  make all image flash'

# go to https://github.com/bitcraze/aideck-gap8-examples/releases
# download aideck_gap8_wifi_img_streamer_with_ap.bin
# paste to aideck-gap8-bootloader
cfloader flash aideck_gap8_wifi_img_streamer_with_ap.bin deck-bcAI:gap8-fw -w radio://0/80/2M

#========crazyflie firmware========
cd ~/Desktop/Bitcraze
sudo apt-get install make gcc-arm-none-eabi
sudo apt install build-essential libncurses5-dev
git clone --recursive https://github.com/bitcraze/crazyflie-firmware.git
cd crazyflie-firmware/
make cf2_defconfig
make -j 12



#========aideck-gap8-examples========
cd ~/Desktop/Bitcraze
git clone https://github.com/bitcraze/aideck-gap8-examples.git
cd aideck-gap8-examples
sudo docker run --rm -v ${PWD}:/module aideck-with-autotiler tools/build/make-example examples/other/wifi-img-streamer image
cfloader flash examples/other/wifi-img-streamer/BUILD/GAP8_V2/GCC_RISCV_FREERTOS/target.board.devices.flash.img deck-bcAI:gap8-fw -w radio://0/80/2M

pip install opencv-python



