#include "beep.h"
#include "log.h"

// 初始化蜂鸣器
void beep_init(void)
{
    pinMode(BEEP_PIN, OUTPUT);
    digitalWrite(BEEP_PIN, 0);
    log_info("蜂鸣器初始化完成 (引脚 %d)", BEEP_PIN);
}

// 打开蜂鸣器
void beep_on(void)
{
    digitalWrite(BEEP_PIN, 1);
    log_debug("蜂鸣器: 开启");
}

// 关闭蜂鸣器
void beep_off(void)
{
    digitalWrite(BEEP_PIN, 0);
    log_debug("蜂鸣器: 关闭");
}
