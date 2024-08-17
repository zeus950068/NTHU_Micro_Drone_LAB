/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * AI-deck GAP8
 *
 * Copyright (C) 2022 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * WiFi image streamer example
 */
#include "pmsis.h"

#include "bsp/bsp.h"
#include "bsp/camera/himax.h"
#include "bsp/buffer.h"
#include "gaplib/jpeg_encoder.h"
#include "stdio.h"

#include "cpx.h"
#include "wifi.h"

#define IMG_ORIENTATION 0x0101
#define CAM_WIDTH 324
#define CAM_HEIGHT 244


#define CHANNELS 1
#define IO RGB888_IO
#define CAT_LEN sizeof(uint32_t)

#define __XSTR(__s) __STR(__s)
#define __STR(__s) #__s


static pi_task_t task1;
static pi_task_t task2;
static unsigned char *imgBuff;
static unsigned char *imgBuff2;
static unsigned char *imgBuff3;
static unsigned char *imgBuff4;
static struct pi_device camera;
static pi_buffer_t buffer;

static EventGroupHandle_t evGroup;
#define CAPTURE_DONE_BIT (1 << 0)

// Performance menasuring variables
static uint32_t start = 0;
static uint32_t start_2 = 0;
static uint32_t captureTime = 0;
static uint32_t captureTime_2 = 0;
static uint32_t transferTime = 0;
static uint32_t encodingTime = 0;
static uint32_t alltime = 0;
// #define OUTPUT_PROFILING_DATA

//#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
//#include <stb_image_resize2.h>

int grid = CAM_WIDTH / 8; // Set grid size as 1/8 of the width
static int captureState = 0;

static int hehe = 0;

//AT_HYPERFLASH_FS_EXT_ADDR_TYPE __PREFIX(_L3_Flash) = 0;

#define IMG_ORIENTATION 0x0101


  ///////////////////////////////
  //       open camera         //
  ///////////////////////////////

static int open_pi_camera_himax(struct pi_device *device)
{
  struct pi_himax_conf cam_conf;

  pi_himax_conf_init(&cam_conf);

  cam_conf.format = PI_CAMERA_QVGA;

  pi_open_from_conf(device, &cam_conf);
  if (pi_camera_open(device))
    return -1;

  // rotate image
  pi_camera_control(device, PI_CAMERA_CMD_START, 0);
  uint8_t set_value = 3;
  uint8_t reg_value;
  pi_camera_reg_set(device, IMG_ORIENTATION, &set_value);
  pi_time_wait_us(1000000);
  pi_camera_reg_get(device, IMG_ORIENTATION, &reg_value);
  if (set_value != reg_value)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to rotate camera image\n");
    return -1;
  }
  pi_camera_control(device, PI_CAMERA_CMD_STOP, 0);
  pi_camera_control(device, PI_CAMERA_CMD_AEG_INIT, 0);

  return 0;
}


  ///////////////////////////////
  //          Optical          //
  ///////////////////////////////
// Bilinear interpolation function
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  ////////////////          floor要改         /////////////////// 
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////

int custom_floor(float x) {
    int i = (int)x;
    return (x < i) ? (i - 1) : i;
}

float bilinear_interpolate(unsigned char* img, float x, float y, int width, int height) {
    //int x0 = (int)floor(x);
    //int x0 = x;
    int x0 = custom_floor(x);
    int x1 = x0 + 1;

    //int y0 = (int)floor(y);
    //int y0 = y;
    int y0 = custom_floor(y);
    int y1 = y0 + 1;

    // Check if coordinates are within the bounds of the image
    if (x0 < 0 || x1 >= width || y0 < 0 || y1 >= height) {
        return 0.0;  // Return zero if out of bounds
    }

    float Ia = img[y0 * width + x0];
    float Ib = img[y0 * width + x1];
    float Ic = img[y1 * width + x0];
    float Id = img[y1 * width + x1];

    float wa = (x1 - x) * (y1 - y);
    float wb = (x - x0) * (y1 - y);
    float wc = (x1 - x) * (y - y0);
    float wd = (x - x0) * (y - y0);

    return wa * Ia + wb * Ib + wc * Ic + wd * Id;
}

void calculate_optical_flow(unsigned char* img1, unsigned char* img2, int width, int height, int x, int y, float* dx, float* dy, int half_patch_size, int iterations) {
    float lastCost = 0.0, cost = 0.0;
    float H[2][2], b[2], J[2];
    float update[2];

    for (int it = 0; it < iterations; it++) {
        memset(H, 0, 4 * sizeof(float));
        memset(b, 0, 2 * sizeof(float));
        cost = 0.0;

        for (int i = -half_patch_size; i <= half_patch_size; i++) {
            for (int j = -half_patch_size; j <= half_patch_size; j++) {
                float Ix = x + i, Iy = y + j;
                float Dx = x + *dx + i, Dy = y + *dy + j;

                float error = bilinear_interpolate(img1, Ix, Iy, width, height) - bilinear_interpolate(img2, Dx, Dy, width, height);
                J[0] = -0.5 * (bilinear_interpolate(img2, Dx + 1, Dy, width, height) - bilinear_interpolate(img2, Dx - 1, Dy, width, height));
                J[1] = -0.5 * (bilinear_interpolate(img2, Dx, Dy + 1, width, height) - bilinear_interpolate(img2, Dx, Dy - 1, width, height));

                b[0] -= error * J[0];
                b[1] -= error * J[1];
                cost += error * error;
                H[0][0] += J[0] * J[0];
                H[0][1] += J[0] * J[1];
                H[1][0] += J[1] * J[0];
                H[1][1] += J[1] * J[1];
            }
        }

        float det = H[0][0] * H[1][1] - H[0][1] * H[1][0];
        if (fabs(det) < 1e-6) {
            break;  // Exit if matrix is singular
        }

        float invH[2][2];  // Inverse of H
        invH[0][0] = H[1][1] / det;
        invH[0][1] = -H[0][1] / det;
        invH[1][0] = -H[1][0] / det;
        invH[1][1] = H[0][0] / det;

        update[0] = invH[0][0] * b[0] + invH[0][1] * b[1];
        update[1] = invH[1][0] * b[0] + invH[1][1] * b[1];

        if (fabs(update[0]) < 1e-2 && fabs(update[1]) < 1e-2) {
            break;  // Break if updates are small
        }

        *dx += update[0];
        *dy += update[1];

        if (it > 0 && cost > lastCost) {
            break;  // Break if cost increases
        }
        lastCost = cost;
    }
}

void draw_line(unsigned char* img, int width, int height, int x0, int y0, int x1, int y1, unsigned char color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2; /* error value e_xy */

    for (;;) {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
            img[y0 * width + x0] = color;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
        if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
    }
}

void calculate_flow_grid(unsigned char* img1, unsigned char* img2, int width, int height, int grid, int half_patch_size, int iterations) {
    for (int y = grid / 2; y < height; y += grid) {
        for (int x = grid / 2; x < width; x += grid) {
            float dx = 0, dy = 0;
            calculate_optical_flow(img1, img2, width, height, x, y, &dx, &dy, half_patch_size, iterations);
            draw_line(img1, width, height, x, y, x + (int)dx, y + (int)dy, 255);
            //cpxPrintToConsole(LOG_TO_CRTP, "Optical flow at (%d, %d): (%f, %f)\n", x, y, dx, dy);
        }
    }
}


static int wifiConnected = 0;
static int wifiClientConnected = 0;

static CPXPacket_t rxp;
void rx_task(void *parameters)
{
  while (1)
  {
    cpxReceivePacketBlocking(CPX_F_WIFI_CTRL, &rxp);

    WiFiCTRLPacket_t * wifiCtrl = (WiFiCTRLPacket_t*) rxp.data;

    switch (wifiCtrl->cmd)
    {
      case WIFI_CTRL_STATUS_WIFI_CONNECTED:
        cpxPrintToConsole(LOG_TO_CRTP, "Wifi connected (%u.%u.%u.%u)\n",
                          wifiCtrl->data[0], wifiCtrl->data[1],
                          wifiCtrl->data[2], wifiCtrl->data[3]);
        wifiConnected = 1;
        break;
      case WIFI_CTRL_STATUS_CLIENT_CONNECTED:
        cpxPrintToConsole(LOG_TO_CRTP, "Wifi client connection status: %u\n", wifiCtrl->data[0]);
        wifiClientConnected = wifiCtrl->data[0];
        break;
      default:
        break;
    }
  }
}

static void capture_done_cb(void *arg)
{
  
  //memcpy(imgBuff, imgBuff3, sizeof(CAM_WIDTH * CAM_HEIGHT));
  //calculate_flow_grid(imgBuff, imgBuff3, CAM_WIDTH, CAM_HEIGHT, grid, 4, 8);
  //calculate_flow_grid(imgBuff, imgBuff2, CAM_WIDTH, CAM_HEIGHT, grid, 4, 5);
  xEventGroupSetBits(evGroup, CAPTURE_DONE_BIT);
}

static void capture_done_cb_2(void *arg)
{
  //memcpy(imgBuff3, imgBuff2, sizeof(CAM_WIDTH * CAM_HEIGHT));
  //memcpy(imgBuff, imgBuff4, sizeof(CAM_WIDTH * CAM_HEIGHT));
  
  calculate_flow_grid(imgBuff3, imgBuff2, CAM_WIDTH, CAM_HEIGHT, grid, 4, 8); // Using the same half_patch_size and iterations as before
  
  xEventGroupSetBits(evGroup, CAPTURE_DONE_BIT);
}

typedef struct
{
  uint8_t magic;
  uint16_t width;
  uint16_t height;
  uint8_t depth;
  uint8_t type;
  uint32_t size;
} __attribute__((packed)) img_header_t;

static jpeg_encoder_t jpeg_encoder;

typedef enum
{
  RAW_ENCODING = 0,
  JPEG_ENCODING = 1
} __attribute__((packed)) StreamerMode_t;

pi_buffer_t header;
uint32_t headerSize;
pi_buffer_t footer;
uint32_t footerSize;
pi_buffer_t jpeg_data;
uint32_t jpegSize;

static StreamerMode_t streamerMode = RAW_ENCODING;

static CPXPacket_t txp;

void createImageHeaderPacket(CPXPacket_t * packet, uint32_t imgSize, StreamerMode_t imgType) {
  img_header_t *imgHeader = (img_header_t *) packet->data;
  imgHeader->magic = 0xBC;
  imgHeader->width = CAM_WIDTH;
  imgHeader->height = CAM_HEIGHT;
  imgHeader->depth = 1;
  imgHeader->type = imgType;
  imgHeader->size = imgSize;
  packet->dataLength = sizeof(img_header_t);
}

void sendBufferViaCPX(CPXPacket_t * packet, uint8_t * buffer, uint32_t bufferSize) {
  uint32_t offset = 0;
  uint32_t size = 0;
  do {
    size = sizeof(packet->data);
    if (offset + size > bufferSize)
    {
      size = bufferSize - offset;
    }
    memcpy(packet->data, &buffer[offset], sizeof(packet->data));
    packet->dataLength = size;
    cpxSendPacketBlocking(packet);
    offset += size;
  } while (size == sizeof(packet->data));
}

#ifdef SETUP_WIFI_AP
void setupWiFi(void) {
  static char ssid[] = "WiFi streaming example";
  cpxPrintToConsole(LOG_TO_CRTP, "Setting up WiFi AP\n");
  // Set up the routing for the WiFi CTRL packets
  txp.route.destination = CPX_T_ESP32;
  rxp.route.source = CPX_T_GAP8;
  txp.route.function = CPX_F_WIFI_CTRL;
  txp.route.version = CPX_VERSION;
  WiFiCTRLPacket_t * wifiCtrl = (WiFiCTRLPacket_t*) txp.data;

  wifiCtrl->cmd = WIFI_CTRL_SET_SSID;
  memcpy(wifiCtrl->data, ssid, sizeof(ssid));
  txp.dataLength = sizeof(ssid);
  cpxSendPacketBlocking(&txp);

  wifiCtrl->cmd = WIFI_CTRL_WIFI_CONNECT;
  wifiCtrl->data[0] = 0x01;
  txp.dataLength = 2;
  cpxSendPacketBlocking(&txp);
}
#endif

void camera_task(void *parameters)
{
  vTaskDelay(2000);

#ifdef SETUP_WIFI_AP
  setupWiFi();
#endif

  cpxPrintToConsole(LOG_TO_CRTP, "Starting camera task...\n");
  uint32_t resolution = CAM_WIDTH * CAM_HEIGHT;
  uint32_t captureSize = resolution * sizeof(unsigned char);
  imgBuff = (unsigned char *)pmsis_l2_malloc(captureSize);
  imgBuff2 = (unsigned char *)pmsis_l2_malloc(captureSize);
  imgBuff3 = (unsigned char *)pmsis_l2_malloc(captureSize);
  imgBuff4 = (unsigned char *)pmsis_l2_malloc(captureSize);
  if (imgBuff == NULL || imgBuff2 == NULL || imgBuff3 == NULL || imgBuff4 == NULL)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to allocate Memory for Image \n");
    return;
  }

  if (open_pi_camera_himax(&camera))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed to open camera\n");
    return;
  }

  struct jpeg_encoder_conf enc_conf;
  jpeg_encoder_conf_init(&enc_conf);
  enc_conf.width = CAM_WIDTH;
  enc_conf.height = CAM_HEIGHT;
  enc_conf.flags = 0; // Move this to the cluster

  if (jpeg_encoder_open(&jpeg_encoder, &enc_conf))
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Failed initialize JPEG encoder\n");
    return;
  }

  pi_buffer_init(&buffer, PI_BUFFER_TYPE_L2, imgBuff);
  pi_buffer_set_format(&buffer, CAM_WIDTH, CAM_HEIGHT, 1, PI_BUFFER_FORMAT_GRAY);

  header.size = 1024;
  header.data = pmsis_l2_malloc(1024);

  footer.size = 10;
  footer.data = pmsis_l2_malloc(10);

  // This must fit the full encoded JPEG
  jpeg_data.size = 1024 * 15;
  jpeg_data.data = pmsis_l2_malloc(1024 * 15);

  if (header.data == 0 || footer.data == 0 || jpeg_data.data == 0) {
    cpxPrintToConsole(LOG_TO_CRTP, "Could not allocate memory for JPEG image\n");
    return;
  }

  jpeg_encoder_header(&jpeg_encoder, &header, &headerSize);
  jpeg_encoder_footer(&jpeg_encoder, &footer, &footerSize);

  pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

  // We're reusing the same packet, so initialize the route once
  cpxInitRoute(CPX_T_GAP8, CPX_T_WIFI_HOST, CPX_F_APP, &txp.route);

  uint32_t imgSize = 0;

  while (1)
  {
    if (wifiClientConnected == 1)
    {
      start = xTaskGetTickCount();
      start_2 = xTaskGetTickCount();
      /*
      pi_camera_capture_async(&camera, imgBuff, resolution, pi_task_callback(&task1, capture_done_cb, NULL));
      pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
      pi_time_wait_us(500000);
      pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);

      pi_camera_capture_async(&camera, imgBuff2, resolution, pi_task_callback(&task2, capture_done_cb_2, NULL));
      pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
      xEventGroupWaitBits(evGroup, CAPTURE_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)portMAX_DELAY);
      pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
      */
      if (captureState == 0)
            {
                // 准备捕获第一张图像
                pi_camera_capture_async(&camera, imgBuff, resolution, pi_task_callback(&task1, capture_done_cb, NULL));
                imgBuff3 = imgBuff ;
                //memcpy(imgBuff3, imgBuff, sizeof(CAM_WIDTH * CAM_HEIGHT));
            }
            else if (captureState == 1)
            {
                // 准备捕获第二张图像
                pi_camera_capture_async(&camera, imgBuff2, resolution, pi_task_callback(&task2, capture_done_cb_2, NULL));
                imgBuff = imgBuff3 ;
                //memcpy(imgBuff, imgBuff3, sizeof(CAM_WIDTH * CAM_HEIGHT));
            }

      pi_camera_control(&camera, PI_CAMERA_CMD_START, 0);
      xEventGroupWaitBits(evGroup, CAPTURE_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)portMAX_DELAY);
      pi_camera_control(&camera, PI_CAMERA_CMD_STOP, 0);
      
      captureTime = xTaskGetTickCount() - start;

      captureState = (captureState + 1) % 2;

      if (streamerMode == JPEG_ENCODING)
      {
        //jpeg_encoder_process_async(&jpeg_encoder, &buffer, &jpeg_data, pi_task_callback(&task1, encoding_done_cb, NULL));
        //xEventGroupWaitBits(evGroup, JPEG_ENCODING_DONE_BIT, pdTRUE, pdFALSE, (TickType_t)portMAX_DELAY);
        //jpeg_encoder_process_status(&jpegSize, NULL);
        start = xTaskGetTickCount();
        jpeg_encoder_process(&jpeg_encoder, &buffer, &jpeg_data, &jpegSize);
        encodingTime = xTaskGetTickCount() - start;

        imgSize = headerSize + jpegSize + footerSize;

        // First send information about the image
        createImageHeaderPacket(&txp, imgSize, JPEG_ENCODING);
        cpxSendPacketBlocking(&txp);

        start = xTaskGetTickCount();
        // First send header
        memcpy(txp.data, header.data, headerSize);
        txp.dataLength = headerSize;
        cpxSendPacketBlocking(&txp);

        // Send image data
        sendBufferViaCPX(&txp, (uint8_t*) jpeg_data.data, jpegSize);

        // Send footer
        memcpy(txp.data, footer.data, footerSize);
        txp.dataLength = footerSize;
        cpxSendPacketBlocking(&txp);

        transferTime = xTaskGetTickCount() - start;
      }
      else
      {
        imgSize = captureSize;
        start = xTaskGetTickCount();

        // First send information about the image
        createImageHeaderPacket(&txp, imgSize, RAW_ENCODING);
        cpxSendPacketBlocking(&txp);

        start = xTaskGetTickCount();
        // Send image
        sendBufferViaCPX(&txp, imgBuff, imgSize);

        transferTime = xTaskGetTickCount() - start;
        alltime = xTaskGetTickCount() - start_2;
      }
// #ifdef OUTPUT_PROFILING_DATA
      cpxPrintToConsole(LOG_TO_CRTP, "capture_opt=%dms, encoding=%d ms (%d bytes), transfer=%d ms, alltime=%d ms\n",
                        captureTime, encodingTime, imgSize, transferTime, alltime);
// #endif
    }
    else
    {
      vTaskDelay(10);
    }
  }
}

#define LED_PIN 2
static pi_device_t led_gpio_dev;
void hb_task(void *parameters)
{
  (void)parameters;
  char *taskname = pcTaskGetName(NULL);

  // Initialize the LED pin
  pi_gpio_pin_configure(&led_gpio_dev, LED_PIN, PI_GPIO_OUTPUT);

  const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

  while (1)
  {
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 1);
    vTaskDelay(xDelay);
    pi_gpio_pin_write(&led_gpio_dev, LED_PIN, 0);
    vTaskDelay(xDelay);
  }
}

void start_example(void)
{
  struct pi_uart_conf conf;
  struct pi_device device;
  pi_uart_conf_init(&conf);
  conf.baudrate_bps = 115200;

  pi_open_from_conf(&device, &conf);
  if (pi_uart_open(&device))
  {
    printf("[UART] open failed !\n");
    pmsis_exit(-1);
  }

  cpxInit();
  cpxEnableFunction(CPX_F_WIFI_CTRL);

  cpxPrintToConsole(LOG_TO_CRTP, "-- Optical_wifi --\n");

  evGroup = xEventGroupCreate();

  BaseType_t xTask;

  xTask = xTaskCreate(hb_task, "hb_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);
  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "HB task did not start !\n");
    pmsis_exit(-1);
  }

  xTask = xTaskCreate(camera_task, "camera_task", configMINIMAL_STACK_SIZE * 4,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "Camera task did not start !\n");
    pmsis_exit(-1);
  }

  xTask = xTaskCreate(rx_task, "rx_task", configMINIMAL_STACK_SIZE * 2,
                      NULL, tskIDLE_PRIORITY + 1, NULL);

  if (xTask != pdPASS)
  {
    cpxPrintToConsole(LOG_TO_CRTP, "RX task did not start !\n");
    pmsis_exit(-1);
  }

  while (1)
  {
    pi_yield();
  }
}

int main(void)
{
  pi_bsp_init();

  // Increase the FC freq to 250 MHz
  pi_freq_set(PI_FREQ_DOMAIN_FC, 250000000);
  pi_pmu_voltage_set(PI_PMU_DOMAIN_FC, 1200);

  return pmsis_kickoff((void *)start_example);
}
