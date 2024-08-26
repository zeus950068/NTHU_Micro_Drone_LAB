## Download Chrome
```bash
cd Downloads
wget -c https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb
sudo dpkg -i google-chrome-stable_current_amd64.deb
sudo apt-get install -f
sudo apt-get remove chrome-chrome-stable
```

## Download Nvidia driver for dual monitor
Check the available drivers for your hardware
```bash
sudo ubuntu-drivers list
```
![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/CRAZYFLIE_UBUNTU_ENV_SET/image/nvidia_driver.png)

Let’s assume we want to install the 535 driver
```bash
sudo ubuntu-drivers install nvidia:535
```

## Install USB Permission
The following steps make it possible to use the USB Radio and Crazyflie 2 over USB without being root.
```bash
sudo groupadd plugdev
sudo usermod -a -G plugdev $USER
```
> [!NOTE]
> You will need to log out and log in again in order to be a member of the plugdev group.

Copy-paste the following in your console, this will create the file /etc/udev/rules.d/99-bitcraze.rules:
```bash
cat <<EOF | sudo tee /etc/udev/rules.d/99-bitcraze.rules > /dev/null
# Crazyradio (normal operation)
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="7777", MODE="0664", GROUP="plugdev"
# Bootloader
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="0101", MODE="0664", GROUP="plugdev"
# Crazyflie (over USB)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5740", MODE="0664", GROUP="plugdev"
EOF
```

You can reload the udev-rules using the following
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Install Docker Engine
### 1. Set up Docker's apt repository
```bash
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
```

### 2. Install the Docker packages
```bash
sudo apt-get install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
```

### 3. Verify that the Docker Engine installation is successful by running the hello-world
```bash
sudo docker run hello-world
```
![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/CRAZYFLIE_UBUNTU_ENV_SET/image/docker%20run.png)

## GreenwWaves GAP_SDK
```bash
cd ~/Desktop
mkdir Greenwaves
cd Greenwaves
git clone https://github.com/GreenWaves-Technologies/gap8_openocd.git
```

### 1. The following packages need to be installed:
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

### 2. Now clone the GAP/RISC-V toolchain
```bash
git clone https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu.git
cd gap_riscv_toolchain_ubuntu
./install.sh
```

### 3. Clone the actual gap_sdk repository
```bash
git clone https://github.com/GreenWaves-Technologies/gap_sdk.git
```

### 4. Configure the SDK
```bash
cd gap_sdk
source sourceme.sh
source configs/ai_deck.sh
```

### 5. Python requirements
```bash
pip3 install -r requirements.txt
pip3 install -r doc/requirements.txt
```

### 6. SDK installation
```bash
make clean
make sdk
```

### 7. Setting up docker and the autotiler
```bash
sudo docker run --rm -it --name myAiDeckContainer bitcraze/aideck
```

> [!WARNING]
> **Open a NEW terminal (DO NOT CLOSE THE ORIGINAL ONE)**
```bash
sudo docker commit myAiDeckContainer aideck-with-autotiler
```

go back to original terminal
```bash
exit
```
![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/CRAZYFLIE_UBUNTU_ENV_SET/image/autotiler.png)

```bash
make autotiler
```

### 8. Install OpenOCD Rules
* Copy openocd udev rules and reload udev rules
```bash
# sudo cp <your openocd path>/openocd/contrib/60-openocd.rules /etc/udev/rules.d
sudo cp ~/Desktop/Greenwaves/gap8_openocd/contrib/60-openocd.rules /etc/udev/rules.d
sudo udevadm control --reload-rules && sudo udevadm trigger
```

* Now, add your user to dialout group.
```bash
# sudo usermod -a -G dialout <username>
sudo usermod -a -G dialout user
```

### 9. Finally try a test project
```bash
cd examples/gap8/basic/helloworld
export GAPY_OPENOCD_CABLE=~/Desktop/Greenwaves/gap8_openocd/tcl/interface/ftdi/olimex-arm-usb-tiny-h.cfg
make clean all run PMSIS_OS=freertos platform=board
```

##
## Install cfclient
```bash
sudo apt install git python3-pip libxcb-xinerama0 libxcb-cursor0
pip3 install --upgrade pip
python3 -m pip install pip setuptools --upgrade
pip3 install cfclient
```
