#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// 摄像头配置
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480
#define CAMERA_FPS 30
#define CAMERA_QUALITY 80
#define CAMERA_DEVICE "/dev/video0"

// 图片存储路径
#define CAMERA_SNAPSHOT_PATH "web/static/images/snapshot.jpg"
#define CAMERA_STREAM_PATH "web/static/images/stream.jpg"

// 摄像头状态
typedef enum {
    CAMERA_STATUS_STOPPED = 0,
    CAMERA_STATUS_RUNNING = 1,
    CAMERA_STATUS_ERROR = 2
} camera_status_t;

// 摄像头配置结构
typedef struct {
    int width;
    int height;
    int fps;
    int quality;
    char device[32];
    int auto_exposure;
    int brightness;
    int contrast;
} camera_config_t;

// 摄像头状态结构
typedef struct {
    camera_status_t status;
    camera_config_t config;
    pthread_t stream_thread;
    int stream_running;
    time_t last_frame_time;
    int frame_count;
} camera_state_t;

// 函数声明
int camera_init(void);
int camera_cleanup(void);
int camera_take_snapshot(void);
int camera_start_stream(void);
int camera_stop_stream(void);
camera_state_t camera_get_state(void);
int camera_set_config(camera_config_t *config);
int camera_is_available(void);

// 内部函数
void* camera_stream_worker(void* arg);
int camera_capture_frame(const char* output_path);
int camera_check_device(void);

#endif // CAMERA_H
