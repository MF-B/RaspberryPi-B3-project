#include "rgb.h"
#include "log.h"

// 初始化RGB LED
void rgb_init(void)
{
    log_debug("RGB初始化开始...");
    
    // 设置引脚为输出模式
    pinMode(RGB_R_PIN, OUTPUT);
    pinMode(RGB_G_PIN, OUTPUT);
    pinMode(RGB_B_PIN, OUTPUT);
    
    // 初始状态：所有LED关闭
    digitalWrite(RGB_R_PIN, 0);
    digitalWrite(RGB_G_PIN, 0);
    digitalWrite(RGB_B_PIN, 0);
    
    log_info("RGB组件初始化完成 (引脚 R:%d G:%d B:%d)", RGB_R_PIN, RGB_G_PIN, RGB_B_PIN);
}

// 开启RGB LED（默认白色）
void rgb_on(void)
{
    digitalWrite(RGB_R_PIN, 1);
    digitalWrite(RGB_G_PIN, 1);
    digitalWrite(RGB_B_PIN, 1);
    log_debug("RGB LED: 开启 (白色)");
}

// 关闭RGB LED
void rgb_off(void)
{
    digitalWrite(RGB_R_PIN, 0);
    digitalWrite(RGB_G_PIN, 0);
    digitalWrite(RGB_B_PIN, 0);
    log_debug("RGB LED: 关闭");
}

// 设置RGB颜色
void rgb_set_color(int red, int green, int blue)
{
    log_debug("设置RGB颜色: R=%d, G=%d, B=%d", red, green, blue);
    digitalWrite(RGB_R_PIN, red);
    digitalWrite(RGB_G_PIN, green);
    digitalWrite(RGB_B_PIN, blue);
}

// 清理RGB组件
void rgb_cleanup(void)
{
    log_debug("RGB组件清理中...");
    rgb_off();
    log_debug("RGB组件清理完成");
}
