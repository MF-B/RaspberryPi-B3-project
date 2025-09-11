#ifndef DISTANCE_H
#define DISTANCE_H

#include <wiringPi.h>
#include <stdio.h>
#include "../config/pins.h"

// 距离传感器核心函数声明
void distance_init(void);
int distance_read(void);

#endif
