**ps. 這是一個針對無法執行AI-deck Workshop Session 4(cannot find labgaptools.a)的解決方法 
https://github.com/pulp-platform/AI-deck-workshop/tree/main/Hands-on/Session%204/GAP8/wifi_jpeg_streamer**

## 1. 安装 SDL2
```bash
sudo apt-get update
sudo apt-get install libsdl2-dev
```
## 2. 安装 OpenCV
```bash
sudo apt-get update
sudo apt-get install libopencv-dev
```
## 3. 下載上方Makefile並取代原本的
Makefile路徑：
```bash
cd /home/user/Desktop/Greenwaves/gap_riscv_toolchain_ubuntu/gap_sdk/libs/frame_streamer
```
## 4. 下載上方pmsis_tools_frame_streamer.cpp並取代原本的
pmsis_tools_frame_streamer.cpp位置:
```bash
cd /home/user/Desktop/Greenwaves/gap_riscv_toolchain_ubuntu/gap_sdk/libs/frame_streamer/python
```
## 5. 回到路徑執行Makefile
```bash
cd /home/user/Desktop/Greenwaves/gap_riscv_toolchain_ubuntu/gap_sdk/libs/frame_streamer
make clean all
```
