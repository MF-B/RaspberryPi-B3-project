#include "rgb.h"

// 初始化RGB LED
void rgb_init(void)
{
    printf("RGB初始化开始...\n");
    
    // 设置引脚为输出模式
    pinMode(RGB_R_PIN, OUTPUT);
    pinMode(RGB_G_PIN, OUTPUT);
    pinMode(RGB_B_PIN, OUTPUT);
    
    // 初始状态：所有LED关闭
    digitalWrite(RGB_R_PIN, 0);
    digitalWrite(RGB_G_PIN, 0);
    digitalWrite(RGB_B_PIN, 0);
    
    printf("RGB组件初始化完成 (引脚 R:%d G:%d B:%d)\n", RGB_R_PIN, RGB_G_PIN, RGB_B_PIN);
}

// 开启RGB LED（默认白色）
void rgb_on(void)
{
    digitalWrite(RGB_R_PIN, 1);
    digitalWrite(RGB_G_PIN, 1);
    digitalWrite(RGB_B_PIN, 1);
    printf("RGB LED: 开启 (白色)\n");
}

// 关闭RGB LED
void rgb_off(void)
{
    digitalWrite(RGB_R_PIN, 0);
    digitalWrite(RGB_G_PIN, 0);
    digitalWrite(RGB_B_PIN, 0);
    printf("RGB LED: 关闭\n");
}

// 设置RGB颜色
void rgb_set_color(int red, int green, int blue)
{
    printf("设置RGB颜色: R=%d, G=%d, B=%d\n", red, green, blue);
    digitalWrite(RGB_R_PIN, red);
    digitalWrite(RGB_G_PIN, green);
    digitalWrite(RGB_B_PIN, blue);
}

// 清理RGB组件
void rgb_cleanup(void)
{
    printf("RGB组件清理中...\n");
    rgb_off();
    printf("RGB组件清理完成\n");
}
