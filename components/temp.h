#ifndef TEMP_H
#define TEMP_H

#include <wiringPi.h>
#include <stdio.h>
#include "../config/pins.h"
#include "../config/sensors.h"

// 核心函数声明
void temp_init(void);
int temp_read(temp_data_t *data);
int temp_read_with_retry(temp_data_t *data, int max_retry);

#endif
