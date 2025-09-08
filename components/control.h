#ifndef CONTROL_H
#define CONTROL_H

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <softPwm.h>

// 引脚定义
#define WHEEL_L 23  // 左轮引脚
#define WHEEL_R 24  // 右轮引脚

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

// 函数声明
void init_wheel(void);
void clean_wheel(void);

// Web API兼容函数
void control_motion(motion_type_t motion, int speed, int duration);
void control_turn_left(int speed, int duration);
void control_turn_right(int speed, int duration);
void control_move_forward(int speed);
void control_move_backward(int speed);
void control_stop(void);

// 状态查询
motion_state_t get_motion_state(void);
void set_wheel_speeds(int left_speed, int right_speed);

#endif // CONTROL_H
