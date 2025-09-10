#include "clock.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// 段码数据 (0-9, A-Z, 空格等)
char segdata[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, // 0~9
    0x40,                                                       // '-' (10)
    0x77,                                                       // A (11)
    0x7c,                                                       // b (12)
    0x39,                                                       // C (13)
    0x5e,                                                       // d (14)
    0x79,                                                       // E (15)
    0x71,                                                       // F (16)
    0x3d,                                                       // G (17)
    0x76,                                                       // H (18)
    0x06,                                                       // I (19)
    0x1e,                                                       // J (20)
    0x76,                                                       // K (same as H) (21)
    0x38,                                                       // L (22)
    0x15,                                                       // M (23)
    0x54,                                                       // n (24)
    0x5c,                                                       // o (25)
    0x73,                                                       // P (26)
    0x67,                                                       // q (27)
    0x50,                                                       // r (28)
    0x6d,                                                       // S (same as 5) (29)
    0x78,                                                       // t (30)
    0x3e,                                                       // U (31)
    0x3e,                                                       // V (same as U) (32)
    0x2a,                                                       // W (33)
    0x76,                                                       // X (same as H) (34)
    0x6e,                                                       // y (35)
    0x5b,                                                       // Z (same as 2) (36)
    0x00,                                                       // 空格 (37)
    0x08                                                        // 下划线 (38)
};

// 内部函数声明
static void clock_start(void);
static void clock_stop(void);
static void clock_write_bit(char bit);
static void clock_write_byte(char data);
static void clock_write_command(char cmd);
static void clock_write_data(char addr, char data);
static void clock_data_display(char *data);

// TM1637开始信号
static void clock_start()
{
    digitalWrite(CLOCK_CLK_PIN, 1);
    delayMicroseconds(140);
    digitalWrite(CLOCK_DIO_PIN, 1);
    delayMicroseconds(140);
    digitalWrite(CLOCK_DIO_PIN, 0);
    delayMicroseconds(140);
    digitalWrite(CLOCK_CLK_PIN, 0);
    delayMicroseconds(140);
}

// TM1637停止信号
static void clock_stop()
{
    digitalWrite(CLOCK_CLK_PIN, 0);
    delayMicroseconds(140);
    digitalWrite(CLOCK_DIO_PIN, 0);
    delayMicroseconds(140);
    digitalWrite(CLOCK_CLK_PIN, 1);
    delayMicroseconds(140);
    digitalWrite(CLOCK_DIO_PIN, 1);
    delayMicroseconds(140);
}

// 写入一个位
static void clock_write_bit(char bit)
{
    digitalWrite(CLOCK_CLK_PIN, 0);
    delayMicroseconds(140);
    if (bit)
        digitalWrite(CLOCK_DIO_PIN, 1);
    else
        digitalWrite(CLOCK_DIO_PIN, 0);
    delayMicroseconds(140);
    digitalWrite(CLOCK_CLK_PIN, 1);
    delayMicroseconds(140);
}

// 写入一个字节
static void clock_write_byte(char data)
{
    for (int i = 0; i < 8; i++)
    {
        clock_write_bit(data & 1);
        data >>= 1;
    }
    
    // 等待ACK
    digitalWrite(CLOCK_CLK_PIN, 0);
    delayMicroseconds(140);
    pinMode(CLOCK_DIO_PIN, INPUT);
    digitalWrite(CLOCK_CLK_PIN, 1);
    delayMicroseconds(140);
    pinMode(CLOCK_DIO_PIN, OUTPUT);
}

// 写入命令
static void clock_write_command(char cmd)
{
    clock_start();
    clock_write_byte(cmd);
    clock_stop();
}

// 写入数据到指定地址
static void clock_write_data(char addr, char data)
{
    clock_start();
    clock_write_byte(0x44); // 数据命令设置
    clock_stop();
    
    clock_start();
    clock_write_byte(0xC0 | addr); // 地址命令设置
    clock_write_byte(data);
    clock_stop();
}

// 显示数据
static void clock_data_display(char *data)
{
    for (int i = 0; i < 4; i++)
    {
        clock_write_data(i, data[i]);
    }
    clock_write_command(0x8F); // 显示控制命令：开启显示，最大亮度
}

// 初始化时钟显示器
void clock_init(void)
{
    pinMode(CLOCK_DIO_PIN, OUTPUT);
    pinMode(CLOCK_CLK_PIN, OUTPUT);
    digitalWrite(CLOCK_DIO_PIN, 1);
    digitalWrite(CLOCK_CLK_PIN, 1);
    
    // 清空显示
    char blank[4] = {0x00, 0x00, 0x00, 0x00};
    clock_data_display(blank);
    
    printf("时钟显示器初始化完成 (引脚 DIO:%d CLK:%d)\n", CLOCK_DIO_PIN, CLOCK_CLK_PIN);
}

// 显示当前时间 (HH:MM格式)
void clock_display_time(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    char display_data[4];
    display_data[0] = segdata[timeinfo->tm_hour / 10];
    display_data[1] = segdata[timeinfo->tm_hour % 10] | 0x80; // 添加冒号
    display_data[2] = segdata[timeinfo->tm_min / 10];
    display_data[3] = segdata[timeinfo->tm_min % 10];
    
    clock_data_display(display_data);
}

// 显示文本（最多4个字符）
void clock_display_text(char *text)
{
    char display_data[4] = {0x00, 0x00, 0x00, 0x00};
    int len = strlen(text);
    if (len > 4) len = 4;
    
    for (int i = 0; i < len; i++)
    {
        char c = text[i];
        if (c >= '0' && c <= '9')
        {
            display_data[i] = segdata[c - '0'];
        }
        else if (c >= 'A' && c <= 'Z')
        {
            display_data[i] = segdata[11 + c - 'A'];
        }
        else if (c >= 'a' && c <= 'z')
        {
            display_data[i] = segdata[11 + c - 'a'];
        }
        else if (c == ' ')
        {
            display_data[i] = segdata[37]; // 空格
        }
        else if (c == '-')
        {
            display_data[i] = segdata[10]; // 横线
        }
        else
        {
            display_data[i] = 0x00; // 不识别的字符显示为空
        }
    }
    
    clock_data_display(display_data);
}

// 清理时钟显示器
void clock_cleanup(void)
{
    printf("时钟显示器清理中...\n");
    
    // 清空显示
    char blank[4] = {0x00, 0x00, 0x00, 0x00};
    clock_data_display(blank);
    
    printf("时钟显示器清理完成\n");
}
