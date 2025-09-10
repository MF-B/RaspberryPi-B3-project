# 树莓派B3项目 - 元件函数命名规范

## 1. 基本命名规则

### 1.1 命名风格
- **格式**: 小写字母 + 下划线 (snake_case)
- **前缀**: 所有函数以组件名作为前缀
- **分隔符**: 使用下划线 `_` 分隔单词

### 1.2 组件前缀对照表
| 组件 | 前缀 | 示例 |
|------|------|------|
| 蜂鸣器 | `beep_` | `beep_init()` |
| RGB LED | `rgb_` | `rgb_init()` |
| 距离传感器 | `distance_` | `distance_init()` |
| 温度传感器 | `temp_` | `temp_init()` |
| 按钮 | `button_` | `button_init()` |
| 时钟显示 | `clock_` | `clock_init()` |
| 摄像头 | `camera_` | `camera_init()` |
| 运动控制 | `wheel_` | `wheel_init()` |

## 2. 核心函数命名模式

### 2.1 必须实现的核心函数
每个组件应该实现以下核心函数：

```c
// 初始化函数 - 设置引脚模式、初始状态等
void <component>_init(void);

// 开启/启动函数 - 激活组件
void <component>_on(void);     // 或 <component>_start()

// 关闭/停止函数 - 停用组件  
void <component>_off(void);    // 或 <component>_stop()
```

### 2.2 可选扩展函数
根据组件功能需求，可以添加：

```c
// 清理函数 - 释放资源
void <component>_cleanup(void);

// 状态查询函数
int <component>_get_status(void);

// 配置函数
void <component>_set_config(config_type config);
```

## 3. 具体组件示例

### 3.1 蜂鸣器组件 (beep)
```c
// 核心函数
void beep_init(void);          // 初始化蜂鸣器
void beep_on(void);            // 开启蜂鸣器
void beep_off(void);           // 关闭蜂鸣器
```

### 3.2 RGB LED组件 (rgb)
```c
// 核心函数
void rgb_init(void);           // 初始化RGB LED
void rgb_on(void);             // 开启RGB LED
void rgb_off(void);            // 关闭RGB LED

// 扩展函数
void rgb_set_color(int r, int g, int b);  // 设置颜色
void rgb_cleanup(void);        // 清理资源
```

### 3.3 温度传感器组件 (temp)
```c
// 核心函数  
void temp_init(void);          // 初始化温度传感器
int temp_read(temp_data_t *data);          // 读取数据
int temp_read_with_retry(temp_data_t *data, int max_retry);  // 带重试读取
```

### 3.4 距离传感器组件 (distance)
```c
// 核心函数  
void distance_init(void);      // 初始化距离传感器
int distance_read(void);       // 读取距离数据
```

### 3.5 按钮组件 (button)
```c
// 核心函数
void button_init(void);        // 初始化按钮
int button_is_pressed(void);   // 检查按钮状态
```

### 3.6 时钟显示组件 (clock)
```c
// 核心函数
void clock_init(void);         // 初始化时钟显示器
void clock_display_time(void); // 显示当前时间
void clock_display_text(char *text); // 显示文本
void clock_cleanup(void);      // 清理资源
```

## 4. 文件组织规范

### 4.1 头文件 (.h)
```c
#ifndef <COMPONENT>_H
#define <COMPONENT>_H

#include <wiringPi.h>
#include <stdio.h>
// 其他必要的头文件...

// 引脚定义
#define <COMPONENT>_PIN xx

// 函数声明
void <component>_init(void);
void <component>_on(void);
void <component>_off(void);

#endif
```

### 4.2 源文件 (.c)
```c
#include "<component>.h"

// 实现函数
void <component>_init(void)
{
    // 初始化代码
    pinMode(<COMPONENT>_PIN, OUTPUT);
    printf("<组件名>初始化完成 (引脚 %d)\n", <COMPONENT>_PIN);
}

void <component>_on(void)
{
    // 开启代码
    digitalWrite(<COMPONENT>_PIN, 1);
    printf("<组件名>: 开启\n");
}

void <component>_off(void)
{
    // 关闭代码
    digitalWrite(<COMPONENT>_PIN, 0);
    printf("<组件名>: 关闭\n");
}
```

## 5. 设计原则

### 5.1 简单性原则
- 只保留实际使用的功能
- 避免过度设计和冗余函数
- 优先实现核心功能

### 5.2 一致性原则
- 所有组件遵循相同的命名规范
- 相似功能使用相似的函数名
- 保持API接口的一致性

### 5.3 可维护性原则
- 函数职责单一，易于理解
- 添加适当的注释和日志输出
- 便于调试和测试

## 6. 版本历史

- v1.0 (2025-09-10): 初始版本，基于beep组件重构经验制定
- v2.0 (2025-09-10): 统一组件命名，删除combo文件夹，简化命名规范
  - 重命名：DHT → temp，usonic → distance，botton → button
  - 移除所有兼容性代码，使用统一命名规范
  - 删除combo文件夹，保持项目结构简洁
