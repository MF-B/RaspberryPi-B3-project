# 树莓派B3硬件控制项目

这是一个基于树莓派B3的硬件控制项目，使用C语言和WiringPi库实现了多个电子元件的控制，包括按钮检测、TM1637四位数码管时钟显示、RGB LED控制、蜂鸣器控制等功能，并提供了**Web界面**和命令行两种控制方式。

## 🌟 新功能：Web界面控制

现在支持通过Web浏览器远程控制硬件设备！

### Web界面特性
- 📱 **响应式设计** - 支持手机、平板、电脑访问
- 🔄 **实时数据** - 自动刷新传感器数据
- 🎨 **现代化UI** - 美观的界面设计
- 🚀 **RESTful API** - 支持HTTP API调用
- ⚡ **快速响应** - 基于C语言的高性能服务器

### 快速开始
```bash
# 编译Web版本
make -f Makefile.web

# 运行Web服务器
sudo ./web_main

# 访问Web界面
http://localhost:8080
```

## 项目结构

```
RaspberryPi-B3-project/
├── main.c              # 原始命令行主程序
├── web_main.c          # Web服务器主程序
├── Makefile            # 原始编译配置
├── Makefile.web        # Web版本编译配置
├── web/                # Web服务器模块
│   ├── http_server.h   # HTTP服务器头文件
│   ├── http_server.c   # HTTP服务器实现
│   ├── api_handlers.c  # API处理器
│   └── static/         # 静态Web文件
│       ├── index.html  # 主页面
│       ├── style.css   # 样式文件
│       └── script.js   # 前端JavaScript
├── components/         # 硬件组件模块
│   ├── beep.c/.h       # 蜂鸣器控制
│   ├── button.c/.h     # 按钮控制
│   ├── clock.c/.h      # TM1637数码管
│   ├── rgb.c/.h        # RGB LED控制
│   ├── temp.c/.h       # 温度传感器
│   ├── distance.c/.h   # 距离传感器
│   ├── control.c/.h    # 运动控制
│   └── camera.cpp/.h   # 摄像头控制
└── README.md           # 项目说明文档
```

## 硬件连接

### 引脚分配表

| 组件 | 引脚类型 | GPIO引脚 | 物理引脚 |
|------|----------|----------|----------|
| 按钮 | 输入 | GPIO 4 | Pin 7 |
| 蜂鸣器 | 输出 | GPIO 18 | Pin 12 |
| TM1637时钟 CLK | 输出 | GPIO 27 | Pin 13 |
| TM1637时钟 DIO | 输出 | GPIO 22 | Pin 15 |
| RGB LED 红色 | 输出 | GPIO 16 | Pin 36 |
| RGB LED 绿色 | 输出 | GPIO 20 | Pin 38 |
| RGB LED 蓝色 | 输出 | GPIO 21 | Pin 40 |

## TODO List

### ✅ 已完成功能
- [x] 实现按钮基本模块
- [x] 实现蜂鸣器基本模块
- [x] 实现数码管基本模块
- [x] 用数码管显示实际时间(时:分)
- [x] 实现LED基本模块
- [x] 实现温湿度传感器模块 (DHT11)
- [x] 实现超声波距离传感器模块
- [x] 实现舵机控制模块
- [x] **实现Web界面控制** ⭐
- [x] **RESTful API接口** ⭐
- [x] **响应式前端设计** ⭐

### 🚧 组合功能
- [x] 实现闹钟功能（设定闹钟时间，到时间蜂鸣器响铃，LED闪烁）
- [x] 实现秒表功能（按钮开始/暂停/重置，数码管显示计时）
- [x] 实现温度显示功能（集成温度传感器，数码管显示温度）
- [x] 实现按钮控制RGB灯颜色切换

### 🔮 待开发功能
- [ ] 添加长按按钮功能（区分短按和长按操作）
- [ ] 实现音乐播放功能（蜂鸣器播放简单旋律）
- [ ] WebSocket实时通信
- [ ] 添加手机APP远程控制
- [ ] 添加定时任务调度器
- [ ] 实现数据记录和图表显示
- [ ] 集成物联网功能（MQTT、云平台对接）

### 🛠️ 代码优化
- [ ] 添加配置文件支持（保存用户设置）
- [ ] 实现非阻塞式按钮检测（使用中断）
- [ ] 添加错误日志记录功能
- [ ] 优化内存使用和资源管理
- [ ] 添加单元测试用例

## 编译和运行

### 环境要求

- 树莓派B3或更高版本
- Raspbian OS
- WiringPi库
- GCC编译器
- libcjson库 (Web版本)

### 安装依赖

```bash
# 更新系统
sudo apt update

# 安装编译工具
sudo apt install build-essential

# 安装WiringPi库
sudo apt install wiringpi

# 安装cJSON库 (Web版本需要)
sudo apt install libcjson-dev

# 验证安装
gpio -v
```

### 编译项目

#### 方式一：使用Web版本Makefile（推荐）
```bash
# 编译所有版本
make -f Makefile.web

# 只编译Web版本
make -f Makefile.web web-only

# 只编译命令行版本
make -f Makefile.web main-only

# 检查依赖
make -f Makefile.web check-deps

# 安装依赖
make -f Makefile.web install-deps
```

#### 方式二：使用原始Makefile
```bash
# 编译命令行版本
make

# 清理编译文件
make clean
```

### 运行程序

#### Web界面版本（推荐）
```bash
# 运行Web服务器
sudo ./web_main

# 然后在浏览器中访问
http://localhost:8080
```

#### 命令行版本
```bash
sudo ./main
```

## 🌐 Web API文档

### 基础URL
```
http://localhost:8080/api/
```

### API端点

#### 1. 获取系统状态
```http
GET /api/status
```
响应示例：
```json
{
  "status": "online",
  "timestamp": 1699123456,
  "components": {
    "rgb": "ready",
    "beep": "ready",
    "temp": "ready",
    "distance": "ready",
    "clock": "ready",
    "control": "ready",
    "camera": "ready"
  }
}
```

#### 2. 获取传感器数据
```http
GET /api/sensors
```
响应示例：
```json
{
  "timestamp": 1699123456,
  "sensors": {
    "dht11": {
      "temperature": 23.5,
      "humidity": 60.2,
      "status": "success"
    },
    "ultrasonic": {
      "distance": 15,
      "status": "success"
    }
  }
}
```

#### 3. 控制RGB LED
```http
POST /api/rgb
Content-Type: application/json

{
  "action": "on",
  "color": "red"  // red, green, blue, white
}
```

```http
POST /api/rgb
Content-Type: application/json

{
  "action": "off"
}
```

#### 4. 控制蜂鸣器
```http
POST /api/beep
Content-Type: application/json

{
  "action": "beep",
  "duration": 1000  // 毫秒
}
```

#### 5. 获取距离数据
```http
GET /api/distance
```

### 使用curl测试API
```bash
# 获取系统状态
curl http://localhost:8080/api/status

# 控制RGB LED
curl -X POST -H "Content-Type: application/json" \
     -d '{"action":"on","color":"red"}' \
     http://localhost:8080/api/rgb

# 控制蜂鸣器
curl -X POST -H "Content-Type: application/json" \
     -d '{"action":"beep","duration":1000}' \
     http://localhost:8080/api/beep

# 控制舵机
```
gpio -v
```

### 编译项目

```bash
# 编译项目
make

# 清理编译文件
make clean

# 重新编译
make rebuild
```

### 运行程序

```bash
# 运行主程序（需要root权限访问GPIO）
sudo ./main_app
```

## 开发说明

### 添加新功能模块

1. 在 `components/` 目录下创建 `.c` 和 `.h` 文件
2. 在头文件中定义引脚和函数接口
3. 在源文件中实现具体功能
4. 更新 `Makefile` 中的源文件列表
5. 在 `main.c` 中集成新模块

### 注意事项

- 程序使用BCM GPIO编号模式

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。
