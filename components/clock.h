#ifndef CLOCK_H
#define CLOCK_H

#include <wiringPi.h>
#include <stdio.h>
#include <time.h>

// TM1637时钟显示器引脚定义
#define CLOCK_DIO_PIN 22
#define CLOCK_CLK_PIN 27

// 段码数据
extern char segdata[];

// 核心函数声明
void clock_init(void);
void clock_display_time(void);
void clock_display_text(char *text);
void clock_cleanup(void);

#endif
