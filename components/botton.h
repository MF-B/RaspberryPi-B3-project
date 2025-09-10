#ifndef BOTTON_H
#define BOTTON_H

#include <wiringPi.h>
#include <stdio.h>

// 按键引脚定义
#define BOTTON_PIN 4

// 按键核心函数声明
void botton_init(void);
int botton_is_pressed(void);

#endif
