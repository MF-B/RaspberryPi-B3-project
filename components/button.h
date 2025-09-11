#ifndef BUTTON_H
#define BUTTON_H

#include <wiringPi.h>
#include <stdio.h>
#include "../config/pins.h"

// 按键核心函数声明
void button_init(void);
int button_is_pressed(void);

#endif
