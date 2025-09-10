#include "botton.h"

// 初始化按键
void botton_init(void)
{
    pinMode(BOTTON_PIN, INPUT);
    printf("按键初始化完成 (引脚 %d)\n", BOTTON_PIN);
}

// 检查按键是否按下（高电平）
int botton_is_pressed(void)
{
    return (digitalRead(BOTTON_PIN) == 1);
}
