#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// 摄像头配置常量
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480
#define CAMERA_FPS 15
#define CAMERA_QUALITY 80
#define CAMERA_DEVICE "/dev/video0"
#define CAMERA_SNAPSHOT_PATH "web/static/images/snapshot.jpg"
#define CAMERA_STREAM_PATH "web/static/images/stream.jpg"

// 摄像头状态枚举
typedef enum {
    CAMERA_STATUS_STOPPED = 0,
    CAMERA_STATUS_RUNNING,
    CAMERA_STATUS_ERROR
} camera_status_t;

// 摄像头配置结构
typedef struct {
    int width;
    int height;
    int fps;
    int quality;
    char device[64];
    int auto_exposure;
    int brightness;
    int contrast;
} camera_config_t;

// 摄像头状态结构
typedef struct {
    camera_status_t status;
    camera_config_t config;
    int stream_running;
    time_t last_frame_time;
    int frame_count;
    char last_error[256];
} camera_state_t;
