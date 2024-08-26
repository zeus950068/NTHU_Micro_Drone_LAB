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
sudo ubuntu-drivers install nvidia:535
