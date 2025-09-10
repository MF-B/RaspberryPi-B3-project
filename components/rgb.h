#ifndef RGB_H
#define RGB_H

#include <wiringPi.h>
#include <stdio.h>

// RGB引脚定义
#define RGB_R_PIN 16
#define RGB_G_PIN 20
#define RGB_B_PIN 21

// RGB核心函数声明
void rgb_init(void);
void rgb_on(void);
void rgb_off(void);
void rgb_set_color(int red, int green, int blue);
void rgb_cleanup(void);

#endif
