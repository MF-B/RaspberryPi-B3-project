#ifndef CONTROL_H
#define CONTROL_H

#include <wiringPi.h>
#include <stdio.h>
#include <softPwm.h>

// 电机引脚定义
#define WHEEL_LP 18  // 左电机正极
#define WHEEL_LN 23  // 左电机负极
#define WHEEL_RP 25  // 右电机正极
#define WHEEL_RN 12  // 右电机负极

// 运动参数
#define WHEEL_MAX_SPEED 100
#define WHEEL_DEFAULT_SPEED 50

// 运动状态枚举
typedef enum {
    MOTION_STOP,
    MOTION_FORWARD,
    MOTION_BACKWARD,
    MOTION_LEFT,
    MOTION_RIGHT,
    MOTION_ACCELERATE,
    MOTION_DECELERATE
} motion_type_t;

// 运动状态结构体
typedef struct {
    int left_speed;
    int right_speed;
    int is_moving;
    motion_type_t current_motion;
} motion_state_t;

// 状态查询函数
motion_state_t get_motion_state(void);

// 核心函数
void wheel_init(void);
void wheel_on(void);
void wheel_off(void);
void wheel_cleanup(void);

// 基本运动控制
void wheel_forward(int speed);
void wheel_backward(int speed);
void wheel_left(int speed);
void wheel_right(int speed);
void wheel_stop(void);
void wheel_spinleft(int speed);
void wheel_spinright(int speed);

#endif
