#ifndef DISTANCE_H
#define DISTANCE_H

#include <wiringPi.h>
#include <stdio.h>

// 距离传感器引脚定义
#define DISTANCE_TRIG_PIN 22
#define DISTANCE_ECHO_PIN 23

// 距离传感器核心函数声明
void distance_init(void);
int distance_read(void);

#endif
