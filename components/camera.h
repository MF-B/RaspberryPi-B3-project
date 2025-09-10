#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// 摄像头状态枚举
typedef enum {
    CAMERA_STATUS_STOPPED = 0,
    CAMERA_STATUS_RUNNING,
    CAMERA_STATUS_ERROR
} camera_status_t;

// 简化的摄像头状态结构
typedef struct {
    camera_status_t status;
    int stream_running;
    int frame_count;
} camera_state_t;

// 函数声明
int camera_init(void);
void camera_cleanup(void);
int camera_take_snapshot(void);
int camera_start_stream(void);
int camera_stop_stream(void);
int camera_is_available(void);
camera_state_t camera_get_state(void);

#ifdef __cplusplus
}
#endif

#endif // CAMERA_H
