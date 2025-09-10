#ifndef TEMP_H
#define TEMP_H

#include <wiringPi.h>
#include <stdio.h>

// 温度传感器引脚定义
#define TEMP_PIN 13

// 返回值定义
#define TEMP_SUCCESS         1   // 成功读取数据
#define TEMP_CHECKSUM_ERROR  0   // 校验和错误
#define TEMP_NO_RESPONSE     2   // 传感器无响应
#define TEMP_TIMEOUT_ERROR   3   // 数据读取超时

// 温度传感器数据结构
typedef struct {
    float humidity;     // 湿度
    float temperature;  // 温度
    unsigned char raw_data[5]; // 原始数据
} temp_data_t;

// 核心函数声明
void temp_init(void);
int temp_read(temp_data_t *data);
int temp_read_with_retry(temp_data_t *data, int max_retry);

#endif
