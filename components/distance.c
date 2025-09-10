#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <time.h>
#include "distance.h"

// 初始化距离传感器
void distance_init(void)
{
    pinMode(DISTANCE_TRIG_PIN, OUTPUT);
    pinMode(DISTANCE_ECHO_PIN, INPUT);
    digitalWrite(DISTANCE_TRIG_PIN, 0);
    printf("距离传感器初始化完成 (引脚 Trig:%d Echo:%d)\n", DISTANCE_TRIG_PIN, DISTANCE_ECHO_PIN);
    delay(1000);
}

// 读取距离数据
int distance_read(void)
{
    time_t t1, t2;
    digitalWrite(DISTANCE_TRIG_PIN, 1);
    delayMicroseconds(10);
    digitalWrite(DISTANCE_TRIG_PIN, 0);
    while (digitalRead(DISTANCE_ECHO_PIN) == 0);
    t1 = micros();
    printf("t1=%ld\n", t1);
    while (digitalRead(DISTANCE_ECHO_PIN) == 1);
    t2 = micros();
    printf("t2=%ld\n", t2);
    digitalWrite(DISTANCE_TRIG_PIN, 0);
    delay(1000);
    return (t2 - t1) * 340 / 20000;
}

