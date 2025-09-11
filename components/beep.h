#ifndef BEEP_H
#define BEEP_H

#include <wiringPi.h>
#include <stdio.h>
#include "../config/pins.h"

// 蜂鸣器核心函数声明
void beep_init(void);
void beep_on(void);
void beep_off(void);

#endif
