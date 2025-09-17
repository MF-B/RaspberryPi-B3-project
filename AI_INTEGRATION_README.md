# AI线路跟踪功能集成说明

本项目已集成AI推理功能，可以使用TensorFlow Lite模型进行实时线路跟踪和车轮控制。

## 功能特性

- **实时AI推理**: 使用TensorFlow Lite在树莓派上进行实时图像推理
- **Python-C集成**: 通过Python C API将Python AI代码集成到C/C++主程序
- **摄像头集成**: 与现有摄像头模块无缝集成
- **车轮控制**: 根据AI推理结果自动控制车轮运动
- **高置信度检测**: 支持置信度阈值设置，提高控制安全性

## 安装依赖

运行安装脚本来设置必要的依赖：

```bash
# 安装AI推理依赖
./install_ai_deps.sh
```

这会安装：
- TensorFlow Lite Runtime
- OpenCV Python绑定
- NumPy
- Python开发头文件

## 模型要求

您需要一个训练好的TensorFlow Lite模型，要求：

- **输入**: 图像 (建议224x224x3，BGR格式)
- **输出**: 4个类别的概率 [left, forward, right, stop]
- **格式**: TensorFlow Lite (.tflite)

将模型文件放在 `model/line_follower.tflite`

## 编译项目

```bash
# 编译主程序
make

# 编译AI测试程序
make test_ai
```

## 使用方法

### 1. 基本AI测试

```bash
# 运行AI推理测试程序
./test_ai [模型文件路径]

# 使用默认模型路径
./test_ai
```

### 2. 集成到现有代码

在您的C/C++代码中包含AI功能：

```c
#include "components/camera.h"
#include "ai_wrapper.h"

int main() {
    // 初始化摄像头
    camera_init();
    
    // 初始化AI功能
    camera_ai_init("model/line_follower.tflite");
    
    // 进行AI推理
    ai_result_t result;
    if (camera_ai_predict_current_frame(&result)) {
        if (result.success && result.meets_threshold) {
            printf("AI预测: %s (置信度: %.3f)\\n", 
                   result.class_name, result.confidence);
            
            // 根据结果控制车轮
            switch (result.class_id) {
                case AI_CLASS_LEFT:
                    wheel_spinleft(30);
                    break;
                case AI_CLASS_FORWARD:
                    wheel_forward(30);
                    break;
                case AI_CLASS_RIGHT:
                    wheel_spinright(30);
                    break;
                case AI_CLASS_STOP:
                default:
                    wheel_stop();
                    break;
            }
        }
    }
    
    // 清理
    camera_ai_cleanup();
    camera_cleanup();
}
```

### 3. 完整线路跟踪示例

```bash
# 编译示例程序
gcc -o ai_line_follower ai_line_follower_example.c target/*.o $(pkg-config --libs opencv4) -lwiringPi -lpthread `python3-config --ldflags --embed`

# 运行线路跟踪
./ai_line_follower [模型文件路径]
```

## API参考

### AI封装函数

```c
// 初始化AI模块
int ai_wrapper_init(const char* model_path);

// 进行AI推理
int ai_wrapper_predict(const unsigned char* image_data, int width, int height, 
                      float confidence_threshold, ai_result_t* result);

// 清理AI模块
void ai_wrapper_cleanup(void);
```

### 摄像头AI函数

```c
// 初始化摄像头AI功能
int camera_ai_init(const char* model_path);

// 对当前帧进行推理
int camera_ai_predict_current_frame(ai_result_t* result);

// 对指定帧进行推理
int camera_ai_predict_from_frame(mjpeg_frame_t* frame, ai_result_t* result);

// 清理AI功能
void camera_ai_cleanup(void);

// 检查AI是否启用
int camera_ai_is_enabled(void);
```

### AI结果结构

```c
typedef struct {
    int success;           // 是否成功 (1=成功, 0=失败)
    int class_id;          // 预测类别ID (0=left, 1=forward, 2=right, 3=stop)
    char class_name[32];   // 类别名称字符串
    float confidence;      // 置信度 (0.0-1.0)
    int meets_threshold;   // 是否满足置信度阈值
    char error_msg[256];   // 错误信息
} ai_result_t;
```

## 性能优化建议

1. **推理频率**: 建议每300-500ms进行一次推理，避免过度占用CPU
2. **图像尺寸**: 使用较小的输入尺寸 (如224x224) 以提高推理速度
3. **置信度阈值**: 设置合适的置信度阈值 (如0.7) 以确保控制安全性
4. **内存管理**: 及时释放图像缓冲区，避免内存泄漏

## 故障排除

### 1. 编译错误

```bash
# 确保安装了Python开发头文件
sudo apt install python3-dev

# 检查Python配置
python3-config --includes
python3-config --ldflags
```

### 2. 运行时错误

```bash
# 检查TensorFlow Lite安装
python3 -c "import tflite_runtime.interpreter as tflite; print('OK')"

# 检查模型文件
ls -la model/line_follower.tflite

# 检查权限
sudo chmod +x install_ai_deps.sh
```

### 3. AI推理失败

- 确保模型文件存在且格式正确
- 检查输入图像尺寸是否匹配模型要求
- 验证摄像头是否正常工作
- 查看错误信息获取详细故障原因

## 扩展功能

您可以轻松扩展AI功能：

1. **多模型支持**: 加载不同的TFLite模型
2. **自定义预处理**: 修改 `ai_inference.py` 中的图像预处理逻辑
3. **新的输出类别**: 添加更多控制动作类别
4. **性能监控**: 添加推理时间和帧率统计

## 许可证

请遵循您使用的TensorFlow Lite模型的许可证要求。
