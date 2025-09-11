#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>
#include <stdlib.h>
#include "../config/sensors.h"

#ifdef __cplusplus
extern "C" {
#endif

// 核心函数声明
int camera_init(void);
void camera_cleanup(void);
int camera_take_snapshot(void);
int camera_is_available(void);
camera_state_t camera_get_state(void);

// 视频流函数声明
int camera_start_stream(void);
int camera_stop_stream(void);
int camera_capture_frame_to_file(const char* filename);
int camera_is_streaming(void);
int camera_set_fps(int fps);

#ifdef __cplusplus
}
#endif

#endif
