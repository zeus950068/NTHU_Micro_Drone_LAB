/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/

 */
#include "pmsis.h"
#include "bsp/bsp.h"
#include "cpx.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "classification.h"

#define CAM_WIDTH 324
#define CAM_HEIGHT 244

#define CHANNELS 1
#define IO RGB888_IO
#define CAT_LEN sizeof(uint32_t)

#define __XSTR(__s) __STR(__s)
#define __STR(__s) #__s


static pi_task_t task1;
static pi_task_t task2;
static unsigned char *cameraBuffer;
static unsigned char *cameraBuffer_2;
// static unsigned char *imageDemosaiced;
// static signed char *imageCropped;
static signed short *Output_1;
static struct pi_device camera;
static struct pi_device cluster_dev;
static struct pi_cluster_task *task;
static struct pi_cluster_conf cluster_conf;

AT_HYPERFLASH_FS_EXT_ADDR_TYPE __PREFIX(_L3_Flash) = 0;

#define IMG_ORIENTATION 0x0101

static int open_camera(struct pi_device *device){
    struct pi_himax_conf cam_conf;

    pi_himax_conf_init(&cam_conf);

    cam_conf.format = PI_CAMERA_QVGA;

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device)){
        return -1;
    }
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    uint8_t set_value = 3;
    uint8_t reg_value;
    pi_camera_reg_set(&camera, IMG_ORIENTATION, &set_value);
    pi_time_wait_us(1000000);
    pi_camera_reg_get(&camera, IMG_ORIENTATION, &reg_value);

    if (set_value != reg_value){
        cpxPrintToConsole(LOG_TO_CRTP,"Failed to rotate camera image\n");
        return -1;
    }
                
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

    pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);
    return 0;
}

  ///////////////////////////////
  //          imflip           //
  ///////////////////////////////
void flipImage(unsigned char* image, int width, int height) {
    int row, col;
    unsigned char temp;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width / 2; col++) {
            // Swap pixels from left to right
            temp = image[row * width + col];
            image[row * width + col] = image[row * width + (width - col - 1)];
            image[row * width + (width - col - 1)] = temp;
        }
    }
}

void input_result(unsigned char* cameraBuffer){
    cpxPrintToConsole(LOG_TO_CRTP, "input[0]: [%d]\n", cameraBuffer[0]);
    cpxPrintToConsole(LOG_TO_CRTP, "input[1]: [%d]\n", cameraBuffer[1]);
    cpxPrintToConsole(LOG_TO_CRTP, "input[2]: [%d]\n", cameraBuffer[2]);
    cpxPrintToConsole(LOG_TO_CRTP, "input[323]: [%d]\n", cameraBuffer[323]);
    cpxPrintToConsole(LOG_TO_CRTP, "input[322]: [%d]\n", cameraBuffer[322]);
    cpxPrintToConsole(LOG_TO_CRTP, "input[321]: [%d]\n", cameraBuffer[321]);
    cpxPrintToConsole(LOG_TO_CRTP, "-----------------------------\n");
    pi_time_wait_us(10000);
}

void output_result(unsigned char* cameraBuffer){
    cpxPrintToConsole(LOG_TO_CRTP, "output[323]: [%d]\n", cameraBuffer[323]);
    cpxPrintToConsole(LOG_TO_CRTP, "output[322]: [%d]\n", cameraBuffer[322]);
    cpxPrintToConsole(LOG_TO_CRTP, "output[321]: [%d]\n", cameraBuffer[321]);
    cpxPrintToConsole(LOG_TO_CRTP, "output[0]: [%d]\n", cameraBuffer[0]);
    cpxPrintToConsole(LOG_TO_CRTP, "output[1]: [%d]\n", cameraBuffer[1]);
    cpxPrintToConsole(LOG_TO_CRTP, "output[2]: [%d]\n", cameraBuffer[2]);
    cpxPrintToConsole(LOG_TO_CRTP, "=============================\n");
    pi_time_wait_us(1000000);
}

static void sss(){
    cameraBuffer_2 = cameraBuffer;
}


static void cam_handler(void *arg){
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    sss();

    input_result(cameraBuffer);
    flipImage(cameraBuffer, CAM_WIDTH, CAM_HEIGHT);
    output_result(cameraBuffer);

    pi_camera_capture_async(&camera, cameraBuffer, CAM_WIDTH * CAM_HEIGHT, pi_task_callback(&task1, cam_handler, NULL));
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
}




int start_example(){
    cpxInit();

    cpxEnableFunction(CPX_F_WIFI_CTRL);

    cpxPrintToConsole(LOG_TO_CRTP, "*** imflip_handler ***\n");

    cpxPrintToConsole(LOG_TO_CRTP, "Starting to open camera\n");

    ///////////////////////////////
    //        first image        //
    ///////////////////////////////
    if (open_camera(&camera)){
        cpxPrintToConsole(LOG_TO_CRTP, "Failed to open camera\n");
        return -1;
    }
    cpxPrintToConsole(LOG_TO_CRTP,"Opened Camera\n");

    cameraBuffer = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH * CAM_HEIGHT) * sizeof(unsigned char));
    if (cameraBuffer == NULL){
        cpxPrintToConsole(LOG_TO_CRTP, "Failed Allocated memory for camera buffer\n");
        return 1;
    }
    cpxPrintToConsole(LOG_TO_CRTP, "Allocated memory for camera buffer\n");

    cameraBuffer_2 = (unsigned char *)pmsis_l2_malloc((CAM_WIDTH * CAM_HEIGHT) * sizeof(unsigned char));
    if (cameraBuffer_2 == NULL){
        cpxPrintToConsole(LOG_TO_CRTP, "Failed Allocated memory for camera buffer\n");
        return 1;
    }
    cpxPrintToConsole(LOG_TO_CRTP, "Allocated memory for camera buffer\n");

    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    pi_camera_capture_async(&camera, cameraBuffer, CAM_WIDTH * CAM_HEIGHT, pi_task_callback(&task1, cam_handler, NULL));
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);

    while (1){
        pi_yield();
    }

    pmsis_l2_malloc_free(cameraBuffer, CAM_WIDTH * CAM_HEIGHT);
    pmsis_l2_malloc_free(cameraBuffer_2, CAM_WIDTH * CAM_HEIGHT);

    return 0;
}


int main(void){
    pi_bsp_init();
    return pmsis_kickoff((void *)start_example);
}
