#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// 摄像头状态枚举
typedef enum {
    CAMERA_STATUS_STOPPED = 0,
    CAMERA_STATUS_RUNNING,
    CAMERA_STATUS_ERROR
} camera_status_t;

// 摄像头状态结构
typedef struct {
    camera_status_t status;
    int stream_running;
    int frame_count;
} camera_state_t;

// MJPEG帧数据结构
typedef struct {
    unsigned char *data;
    size_t size;
    int width;
    int height;
} mjpeg_frame_t;

// 核心函数声明
int camera_init(void);
void camera_cleanup(void);
int camera_take_snapshot(void);
int camera_is_available(void);
camera_state_t camera_get_state(void);

// MJPEG视频流函数
int camera_start_stream(void);
int camera_stop_stream(void);
int camera_get_mjpeg_frame(mjpeg_frame_t *frame);
void camera_free_frame(mjpeg_frame_t *frame);
int camera_is_streaming(void);

#ifdef __cplusplus
}
#endif

#endif
