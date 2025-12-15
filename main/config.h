#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_SSID     "Xiaomi_DD71"
#define CONFIG_PASSWORD "95078191"

// Edge Impulse config
#define ROI_X           0
#define ROI_Y           100
#define ROI_W           240
#define ROI_H           48
#define ROI_SIZE        (ROI_W * ROI_H * 3)
#define DIGIT_W         48//EI_CLASSIFIER_INPUT_WIDTH
#define DIGIT_H         48//EI_CLASSIFIER_INPUT_HEIGHT
#define DIGIT_SIZE      (DIGIT_W * DIGIT_H * 3)
#define THRESHOLD_VAL   0.6f


#define IMAGES_DIR      "/sdcard/images"
#define ROI_PATH        "/images/roi_%d.jpg"
#define DIGIT_PATH      "/images/digit%d_%d.jpg"

// Camera pins for XIAO ESP32S3
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#endif