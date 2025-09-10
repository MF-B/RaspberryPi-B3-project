#ifndef BUTTON_H
#define BUTTON_H

#include <wiringPi.h>
#include <stdio.h>

// 按键引脚定义
#define BUTTON_PIN 4

// 按键核心函数声明
void button_init(void);
int button_is_pressed(void);

#endif
