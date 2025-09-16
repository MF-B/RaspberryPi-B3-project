#ifndef AI_H
#define AI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// AI模块状态定义
typedef enum {
    AI_STATE_STOPPED,
    AI_STATE_RUNNING,
    AI_STATE_ERROR
} ai_state_t;

// AI寻迹方向定义
typedef enum {
    AI_DIRECTION_STOP,
    AI_DIRECTION_FORWARD,
    AI_DIRECTION_LEFT,
    AI_DIRECTION_RIGHT
} ai_direction_t;

// AI状态结构体
typedef struct {
    ai_state_t state;
    int is_enabled;
    ai_direction_t current_direction;
    float confidence;
    int frame_count;
    time_t last_update;
} ai_status_t;

// AI模块初始化和清理
int ai_init(void);
void ai_cleanup(void);

// AI功能控制
int ai_start(void);
int ai_stop(void);
int ai_enable(void);
int ai_disable(void);

// 状态查询
ai_status_t ai_get_status(void);
int ai_is_running(void);
int ai_is_enabled(void);

// 获取AI预测结果
ai_direction_t ai_get_direction(void);
float ai_get_confidence(void);

// AI模块配置
int ai_set_model_path(const char* path);
int ai_set_camera_index(int index);

// 调试和日志
void ai_set_debug_mode(int enabled);
void ai_print_status(void);

#endif
