#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wiringPi.h>
#include "temp.h"

// 内部函数声明
static int temp_scan(void);
static void temp_reset(void);
static char temp_read_bit(void);
static char temp_read_byte(void);
static unsigned char temp_read_data(char *buff);

// 扫描传感器状态
static int temp_scan()
{
    return digitalRead(TEMP_PIN);
}

// 重置传感器
static void temp_reset()
{
    pinMode(TEMP_PIN, OUTPUT);
    digitalWrite(TEMP_PIN, 1);
    delay(100); // 确保传感器准备好
    
    // 拉低信号至少18ms
    digitalWrite(TEMP_PIN, 0);
    delay(20);
    
    // 拉高信号20-40微秒
    digitalWrite(TEMP_PIN, 1);
    delayMicroseconds(30);
    
    // 设置为输入模式，等待传感器响应
    pinMode(TEMP_PIN, INPUT);
}

// 读取一个数据位
static char temp_read_bit()
{
    int timeout = 0;
    int high_time = 0;
    
    // 等待信号变高，带超时检测
    while (temp_scan() == 0)
    {
        delayMicroseconds(1);
        timeout++;
        if (timeout > 200) // 200微秒超时
            return 2; // 超时错误
    }
    
    // 测量高电平持续时间
    while (temp_scan() == 1)
    {
        delayMicroseconds(1);
        high_time++;
        if (high_time > 200) // 防止死循环
            return 2;
    }
    
    // 根据高电平持续时间判断数据位
    // DHT11: 26-28微秒表示0，70微秒表示1
    if (high_time > 40)
    {
        return 1; // 数据位为1
    }
    else
    {
        return 0; // 数据位为0
    }
}

// 读取一个字节
static char temp_read_byte()
{
    char i;
    unsigned char data = 0, retval;

    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        retval = temp_read_bit();
        if (retval == 2)
        {
            return 0xcc;
        }
        data |= retval;
    }
    return data;
}

// 读取传感器数据
static unsigned char temp_read_data(char *buff)
{
    int i = 0;  
    int timeout = 0;
    
    temp_reset();
    
    // 等待传感器响应信号（应该拉低80微秒）
    timeout = 0;
    while (temp_scan() == 1)
    {
        delayMicroseconds(1);
        timeout++;
        if (timeout > 500) // 增加超时时间到500微秒
            return TEMP_NO_RESPONSE; // 传感器无响应
    }
    
    // 等待响应信号结束（拉高80微秒）
    timeout = 0;
    while (temp_scan() == 0)
    {
        delayMicroseconds(1);
        timeout++;
        if (timeout > 500)
            return TEMP_NO_RESPONSE;
    }
    
    // 等待数据传输开始
    timeout = 0;
    while (temp_scan() == 1)
    {
        delayMicroseconds(1);
        timeout++;
        if (timeout > 500)
            return TEMP_NO_RESPONSE;
    }
    
    // 读取40位数据（5字节）
    for (i = 0; i < 5; i++)
    {
        buff[i] = temp_read_byte();
        if (buff[i] == 0xcc)
        {
            return TEMP_TIMEOUT_ERROR; // 数据读取超时
        }
    }
    
    // 设置引脚为输出模式并拉高
    pinMode(TEMP_PIN, OUTPUT);
    digitalWrite(TEMP_PIN, 1);
    
    // 校验和检查
    if (buff[4] == ((buff[0] + buff[1] + buff[2] + buff[3]) & 0xff))
    {
        return TEMP_SUCCESS;
    }
    else
    {
        return TEMP_CHECKSUM_ERROR;
    }
}

// 初始化温度传感器
void temp_init(void)
{
    // 设置初始状态
    pinMode(TEMP_PIN, OUTPUT);
    digitalWrite(TEMP_PIN, 1);
    delay(1000); // 等待传感器稳定
    
    printf("温度传感器初始化完成 (引脚 %d)\n", TEMP_PIN);
}

// 读取传感器数据
int temp_read(temp_data_t *data)
{
    char buffer[5];
    unsigned char result = temp_read_data(buffer);
    
    if (result == TEMP_SUCCESS && data != NULL)
    {
        // 解析数据
        data->humidity = buffer[0] + buffer[1] * 0.1;
        data->temperature = buffer[2] + buffer[3] * 0.1;
        
        // 保存原始数据
        for (int i = 0; i < 5; i++) {
            data->raw_data[i] = buffer[i];
        }
    }
    
    return result;
}

// 带重试的读取函数
int temp_read_with_retry(temp_data_t *data, int max_retry)
{
    int retry_count = 0;
    int result;
    
    if (data == NULL || max_retry < 1) {
        return TEMP_TIMEOUT_ERROR;
    }
    
    do {
        result = temp_read(data);
        
        if (result == TEMP_SUCCESS) {
            return TEMP_SUCCESS;
        }
        
        retry_count++;
        if (retry_count < max_retry) {
            delay(100); // 短暂等待后重试
        }
    } while (retry_count < max_retry);
    
    return result; // 返回最后一次的错误码
}
