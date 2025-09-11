#ifndef CONTROL_H
#define CONTROL_H

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <softPwm.h>
#include "../config/pins.h"
#include "../config/motion.h"

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
