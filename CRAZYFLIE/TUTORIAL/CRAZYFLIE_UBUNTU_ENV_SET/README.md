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
```bash
sudo groupadd plugdev
sudo usermod -a -G plugdev $USER
```

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

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```
