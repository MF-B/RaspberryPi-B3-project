#include "button.h"
#include "../log.h"

// 初始化按键
void button_init(void)
{
    pinMode(BUTTON_PIN, INPUT);
    log_info("按键初始化完成 (引脚 %d)", BUTTON_PIN);
}

// 检查按键是否按下（高电平）
int button_is_pressed(void)
{
    return (digitalRead(BUTTON_PIN) == 1);
}
