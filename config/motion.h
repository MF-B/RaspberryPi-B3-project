#ifndef MOTION_H
#define MOTION_H

// ====================
// 运动控制配置定义
// ====================

// 运动参数
#define MAX_SPEED 100
#define MIN_SPEED 0
#define STEP_SIZE 5
#define TURN_SPEED 60
#define TURN_DURATION 1000  // 转向持续时间(毫秒)

// 运动方向枚举
typedef enum {
    MOTION_STOP = 0,
    MOTION_FORWARD,
    MOTION_BACKWARD,
    MOTION_LEFT,
    MOTION_RIGHT,
    MOTION_ACCELERATE,
    MOTION_DECELERATE
} motion_type_t;

// 运动状态结构
typedef struct {
    int left_speed;
    int right_speed;
    motion_type_t current_motion;
    int is_moving;
} motion_state_t;

#endif
