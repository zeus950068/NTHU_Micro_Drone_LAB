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
step1: 
![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/install%20aideck/build%20firmware(1).png)
![image](https://github.com/zeus950068/NTHU_Micro_Drone_LAB/blob/main/CRAZYFLIE/TUTORIAL/install%20aideck/build%20firmware(2).png)
