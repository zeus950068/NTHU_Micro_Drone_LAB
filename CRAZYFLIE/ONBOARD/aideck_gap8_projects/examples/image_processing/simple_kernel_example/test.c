/*
 * Copyright (C) 2019 GreenWaves Technologies
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 *
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 * First update: bitcraze
 * Second update (paralellization, adding inerting kernel): Hanna Müller, ETH (hanmuell@iis.ee.ethz.ch)  
 */

// This example shows how to strean the capture of a camera image using
// small buffers.
// This is using aynchronous transfers to make sure a second buffer is always ready
// when the current is finished, to not lose any data.

//===================include drivers====================
#include "pmsis.h"
#include "bsp/bsp.h"
#include "bsp/camera.h"
#include "bsp/camera/himax.h"
//===================include drivers====================


//==============include image IO library================
#include "gaplib/ImgIO.h"
//==============include image IO library================


//===========include own demosaicking func.=============
#include "img_proc.h"
//===========include own demosaicking func.=============


//===============define acquisition size================
#define WIDTH    324

#ifdef QVGA_MODE
    #define HEIGHT   244
#else
    #define HEIGHT   324
#endif

#define BUFF_SIZE (WIDTH*HEIGHT)
//===============define acquisition size================


//========define variables - place buffer in L2=========
PI_L2 unsigned char *buff;

#if (defined(DEMOSAICKING_KERNEL_FC) || defined(DEMOSAICKING_KERNEL_CLUSTER))
    PI_L2 unsigned char *buff_demosaick;
#endif

#if (defined(INVERTING_KERNEL_FC) || defined(INVERTING_KERNEL_CLUSTER))
    PI_L2 unsigned char *buff_inverting;
#endif
//========define variables - place buffer in L2=========


static struct pi_device camera;
static volatile int done;

#if (defined(DEMOSAICKING_KERNEL_CLUSTER) || defined(INVERTING_KERNEL_CLUSTER))
    struct pi_device cluster_dev = {0};
    struct pi_cluster_conf cl_conf = {0};

    /* Cluster main entry, executed by core 0. */
    void cluster_delegate(void *arg){
        /* Task dispatch to cluster cores. */
        #ifdef DEMOSAICKING_KERNEL_CLUSTER
            pi_cl_team_fork(pi_cl_cluster_nb_cores(), cluster_demosaicking, arg);
        #elif defined(INVERTING_KERNEL_CLUSTER)
            pi_cl_team_fork(pi_cl_cluster_nb_cores(), cluster_inverting, arg);
        #else
            printf("something went wrong, no kernel found to execute\n");
        #endif
    }
#endif

// Asynchronus capture – can queue buffer before starting camera
static void handle_transfer_end(void *arg){
    done = 1;
}


//==============open and initialize camera===============
static int open_camera(struct pi_device *device){
    printf("Opening Himax camera\n");
    struct pi_himax_conf cam_conf;
    pi_himax_conf_init(&cam_conf);

    #if defined(QVGA_MODE)
        cam_conf.format = PI_CAMERA_QVGA;
    #endif

    pi_open_from_conf(device, &cam_conf);
    if (pi_camera_open(device))
        return -1;
    pi_camera_control(device, PI_CAMERA_CMD_START, 0);
    uint8_t set_value=3;
    uint8_t reg_value;
    pi_camera_reg_set(device, IMG_ORIENTATION, &set_value);
    pi_time_wait_us(1000000);
    pi_camera_reg_get(device, IMG_ORIENTATION, &reg_value);
    if (set_value!=reg_value){
        printf("Failed to rotate camera image\n");
        return -1;
    }

    pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);

    return 0;
}
//==============open and initialize camera===============



int test_camera(){
    //Set Cluster Frequency to max
    pi_freq_set(PI_FREQ_DOMAIN_CL, 50000000);
    pi_freq_set(PI_FREQ_DOMAIN_FC, 50000000);
    printf("Entering main controller\n");

    #ifdef ASYNC_CAPTURE
        printf("Testing async camera capture\n");
    #else
        printf("Testing normal camera capture\n");
    #endif

    #ifdef INVERTING_KERNEL_FC
        printf("Testing inverting kernel on fabric controller\n");
    #endif

    #ifdef INVERTING_KERNEL_CLUSTER
        printf("Testing inverting kernel on cluster\n");
    #endif

    #ifdef DEMOSAICKING_KERNEL_FC
        printf("Testing demosaicking kernel on fabric controller\n");
    #endif

    #ifdef DEMOSAICKING_KERNEL_CLUSTER
        printf("Testing demosaicking kernel on cluster\n");
    #endif

    // Open the Himax camera
    if (open_camera(&camera)){
        printf("Failed to open camera\n");
        pmsis_exit(-1);
    }


    //======================configure camera register========================
    // Rotate camera orientation
    uint8_t set_value;
    uint8_t reg_value;

    #ifdef QVGA_MODE
        set_value=1;
        pi_camera_reg_set(&camera, QVGA_WIN_EN, &set_value);
        pi_camera_reg_get(&camera, QVGA_WIN_EN, &reg_value);
        printf("qvga window enabled %d\n",reg_value);
    #endif

    #ifndef ASYNC_CAPTURE
        set_value=0;                                                                                                                                                                   
        pi_camera_reg_set(&camera, VSYNC_HSYNC_PIXEL_SHIFT_EN, &set_value);
        pi_camera_reg_get(&camera, VSYNC_HSYNC_PIXEL_SHIFT_EN, &reg_value);
        printf("vsync hsync pixel shift enabled %d\n",reg_value);
    #endif
    //======================configure camera register========================


    //=====================allocated buffers in L2===========================
    // Reserve buffer space for image
    buff = pmsis_l2_malloc(BUFF_SIZE);
    if (buff == NULL){ return -1;}

    #if (defined(DEMOSAICKING_KERNEL_FC) || defined(DEMOSAICKING_KERNEL_CLUSTER))
        #ifdef COLOR_IMAGE
            buff_demosaick = pmsis_l2_malloc(BUFF_SIZE*3);
        #else
            buff_demosaick = pmsis_l2_malloc(BUFF_SIZE);
        #endif

        if (buff_demosaick == NULL){ return -1;}
    #endif

    #if (defined(INVERTING_KERNEL_FC) || defined(INVERTING_KERNEL_CLUSTER))
        buff_inverting = pmsis_l2_malloc(BUFF_SIZE);
        if (buff_inverting == NULL){ return -1;}
    #endif

    printf("Initialized buffers\n");
    //=====================allocated buffers in L2===========================



    #if (defined(DEMOSAICKING_KERNEL_CLUSTER) || defined(INVERTING_KERNEL_CLUSTER))
        /* Init cluster configuration structure. */
        pi_cluster_conf_init(&cl_conf);
        cl_conf.id = 0;                /* Set cluster ID. */
        /* Configure & open cluster. */
        pi_open_from_conf(&cluster_dev, &cl_conf);
        if (pi_cluster_open(&cluster_dev)){
            printf("Cluster open failed !\n");
            pmsis_exit(-1);
        }

        /* Prepare cluster task. */
        // 这个结构体用于定义一个任务（task），它将被发送到 GAP8 处理器的计算集群中执行
        struct pi_cluster_task cl_task = {0};
        cl_task.entry = cluster_delegate;


        #ifdef INVERTING_KERNEL_CLUSTER
            plp_example_kernel_instance_i32 args = {
                .srcBuffer = buff,                  // 指向原图像数据的缓冲区
                .resBuffer = buff_inverting,        // 指向处理后图像数据的缓冲区
                .width = WIDTH, 
                .height = HEIGHT, 
                .nPE = pi_cl_cluster_nb_cores(),    // 处理器内核的数量，用于并行处理
                .grayscale = 1                      // 表示图像是否为灰度图
            };
        #elif defined(DEMOSAICKING_KERNEL_CLUSTER)
            plp_example_kernel_instance_i32 args = {
                .srcBuffer = buff, 
                .resBuffer = buff_demosaick, 
                .width = WIDTH, 
                .height = HEIGHT, 
                .nPE = pi_cl_cluster_nb_cores(), 
                #ifdef COLOR_IMAGE
                    .grayscale = 0
                #else
                    .grayscale = 1
                #endif
            };
        #else
            printf("something went wrong, no kernel found to execute\n");
        #endif
        
        // 结构体实例 args 在被设置好后，其地址通过如下代码传递给集群任务
        // cl_task.arg 是一个 void* 指针，用于传递任意类型的数据。
        // 在这个例子中，它指向 args 结构体的地址，
        // 以便在集群中的 cluster_delegate 函数调用时，能够访问这些参数
        cl_task.arg = (void*)&args; 
    #endif

    #ifdef ASYNC_CAPTURE
        // Start up async capture task
        done = 0;
        pi_task_t task;
        // Asynchronus capture – can queue buffer before starting camera (pi_task_callback)
        pi_camera_capture_async(&camera, buff, BUFF_SIZE, pi_task_callback(&task, handle_transfer_end, NULL));
    #endif

    // Start the camera
    pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    #ifdef ASYNC_CAPTURE
        // Wait for capture to end (pi_yield() blocks until an event happens)
        while(!done){
            pi_yield();
        }
    #else
        // blocking capture
        pi_camera_capture(&camera, buff, BUFF_SIZE);
    #endif

    // Stop the camera and immediately close it
    pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    pi_camera_close(&camera);

    uint32_t time_before = pi_time_get_us();
    #ifdef DEMOSAICKING_KERNEL_FC
        //====================apply a kernel======================
        #ifdef COLOR_IMAGE
            demosaicking(buff, buff_demosaick, WIDTH, HEIGHT, 0);
        #else
            demosaicking(buff, buff_demosaick, WIDTH, HEIGHT, 1);
        #endif
        //====================apply a kernel======================
    #endif

    #if (defined(DEMOSAICKING_KERNEL_CLUSTER) || defined(INVERTING_KERNEL_CLUSTER))
        pi_cluster_send_task_to_cl(&cluster_dev, &cl_task);
    #endif

    #ifdef INVERTING_KERNEL_FC
        inverting(buff, buff_inverting, WIDTH, HEIGHT);
    #endif
    uint32_t time_after = pi_time_get_us();
    printf("Computation time for kernel: %dus \n", time_after - time_before);


    //==============================Write image over openOCD/JTAG to a file on the computer==============================
    // Write to file
    #if (defined(DEMOSAICKING_KERNEL_FC) || defined(DEMOSAICKING_KERNEL_CLUSTER))
        #ifdef COLOR_IMAGE
            WriteImageToFile("../../../img_color.ppm", WIDTH, HEIGHT, sizeof(uint32_t), buff_demosaick, RGB888_IO);
        #else
            WriteImageToFile("../../../img_gray.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff_demosaick, GRAY_SCALE_IO);
        #endif
    #endif

    #if (defined(INVERTING_KERNEL_FC) || defined(INVERTING_KERNEL_CLUSTER))
        WriteImageToFile("../../../img_inverted.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff_inverting, GRAY_SCALE_IO);
    #endif

    WriteImageToFile("../../../img_raw.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff, GRAY_SCALE_IO );
    // //==============================Write image over openOCD/JTAG to a file on the computer==============================


    //===========================================================================================================================================
    //===========================================================WHILE(1) CASE===================================================================
    // // Start the camera
    // pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
    // while(1){
    //     // blocking capture
    //     pi_camera_capture(&camera, buff, BUFF_SIZE);

    //     uint32_t time_before = pi_time_get_us();
    //     #ifdef DEMOSAICKING_KERNEL_FC
    //         //====================apply a kernel======================
    //         #ifdef COLOR_IMAGE
    //             demosaicking(buff, buff_demosaick, WIDTH, HEIGHT, 0);
    //         #else
    //             demosaicking(buff, buff_demosaick, WIDTH, HEIGHT, 1);
    //         #endif
    //         //====================apply a kernel======================
    //     #endif

    //     #if (defined(DEMOSAICKING_KERNEL_CLUSTER) || defined(INVERTING_KERNEL_CLUSTER))
    //         pi_cluster_send_task_to_cl(&cluster_dev, &cl_task);
    //     #endif

    //     #ifdef INVERTING_KERNEL_FC
    //         inverting(buff, buff_inverting, WIDTH, HEIGHT);
    //     #endif
    //     uint32_t time_after = pi_time_get_us();
    //     printf("Computation time for kernel: %dus \n", time_after - time_before);


    //     //==============================Write image over openOCD/JTAG to a file on the computer==============================
    //     // Write to file
    //     #if (defined(DEMOSAICKING_KERNEL_FC) || defined(DEMOSAICKING_KERNEL_CLUSTER))
    //         #ifdef COLOR_IMAGE
    //             WriteImageToFile("../../../img_color.ppm", WIDTH, HEIGHT, sizeof(uint32_t), buff_demosaick, RGB888_IO);
    //         #else
    //             WriteImageToFile("../../../img_gray.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff_demosaick, GRAY_SCALE_IO);
    //         #endif
    //     #endif

    //     #if (defined(INVERTING_KERNEL_FC) || defined(INVERTING_KERNEL_CLUSTER))
    //         WriteImageToFile("../../../img_inverted.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff_inverting, GRAY_SCALE_IO);
    //     #endif

    //     WriteImageToFile("../../../img_raw.ppm", WIDTH, HEIGHT, sizeof(uint8_t), buff, GRAY_SCALE_IO );
    // }
    // // Stop the camera and immediately close it
    // pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
    // pi_camera_close(&camera);
    //==============================Write image over openOCD/JTAG to a file on the computer==============================
    //===========================================================WHILE(1) CASE===================================================================
    //===========================================================================================================================================


    pmsis_exit(0);
}

int main(void){
    printf("\n\t*** PMSIS Camera Example ***\n\n");
    return pmsis_kickoff((void *) test_camera);
}
