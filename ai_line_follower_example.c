/*
 * AI线路跟踪集成示例
 * 在主程序中集成AI推理和车轮控制功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <wiringPi.h>
#include "components/camera.h"
#include "components/control.h"
#include "ai_wrapper.h"

// 全局标志
static volatile int g_running = 1;

// 信号处理函数
void signal_handler(int sig) {
    printf("\n接收到信号 %d，正在停止...\n", sig);
    g_running = 0;
}

// AI线路跟踪主函数
int ai_line_following_mode(void) {
    printf("=== AI线路跟踪模式 ===\n");
    
    ai_result_t result;
    int inference_count = 0;
    int success_count = 0;
    
    while (g_running) {
        // 进行AI推理
        if (camera_ai_predict_current_frame(&result)) {
            inference_count++;
            
            if (result.success && result.meets_threshold) {
                success_count++;
                
                printf("AI推理 #%d: %s (置信度: %.3f)\n", 
                       inference_count, result.class_name, result.confidence);
                
                // 根据AI结果控制车轮
                switch (result.class_id) {
                    case AI_CLASS_LEFT:
                        printf("→ 执行左转\n");
                        wheel_stop();
                        delay(50);
                        wheel_spinleft(25);
                        break;
                        
                    case AI_CLASS_FORWARD:
                        printf("→ 执行直行\n");
                        wheel_forward(30);
                        break;
                        
                    case AI_CLASS_RIGHT:
                        printf("→ 执行右转\n");
                        wheel_stop();
                        delay(50);
                        wheel_spinright(25);
                        break;
                        
                    case AI_CLASS_STOP:
                    default:
                        printf("→ 执行停止\n");
                        wheel_stop();
                        break;
                }
            } else {
                printf("AI推理 #%d: 置信度不足 (%.3f) 或失败\n", 
                       inference_count, result.confidence);
                wheel_stop();
            }
        } else {
            printf("AI推理失败: %s\n", result.error_msg);
            wheel_stop();
        }
        
        // 控制推理频率 (每300ms推理一次)
        delay(300);
    }
    
    // 停止所有运动
    wheel_stop();
    
    printf("\nAI线路跟踪统计:\n");
    printf("  总推理次数: %d\n", inference_count);
    printf("  成功次数: %d\n", success_count);
    if (inference_count > 0) {
        printf("  成功率: %.1f%%\n", success_count * 100.0 / inference_count);
    }
    
    return 0;
}

// 主函数 - 集成AI功能的示例
int main(int argc, char* argv[]) {
    printf("=== 树莓派AI线路跟踪系统 ===\n");
    
    // 解析命令行参数
    const char* model_path = (argc > 1) ? argv[1] : "model/line_follower.tflite";
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化wiringPi
    printf("初始化wiringPi...\n");
    if (wiringPiSetup() == -1) {
        printf("错误: wiringPi初始化失败\n");
        return 1;
    }
    
    // 初始化车轮控制
    printf("初始化车轮控制...\n");
    wheel_init();
    
    // 初始化摄像头
    printf("初始化摄像头...\n");
    if (!camera_init()) {
        printf("错误: 摄像头初始化失败\n");
        return 1;
    }
    
    // 初始化AI推理功能
    printf("初始化AI推理功能 (模型: %s)...\n", model_path);
    if (!camera_ai_init(model_path)) {
        printf("错误: AI推理功能初始化失败\n");
        printf("请确保:\n");
        printf("1. 模型文件存在: %s\n", model_path);
        printf("2. 已安装tflite-runtime: pip3 install tflite-runtime\n");
        printf("3. Python开发环境正确安装\n");
        
        camera_cleanup();
        return 1;
    }
    
    printf("所有模块初始化完成！\n");
    printf("按Ctrl+C停止系统\n\n");
    
    // 运行AI线路跟踪
    ai_line_following_mode();
    
    // 清理资源
    printf("正在清理资源...\n");
    camera_ai_cleanup();
    camera_cleanup();
    wheel_stop();
    
    printf("系统已停止\n");
    return 0;
}

// 演示如何在其他函数中调用AI推理
void example_integration_in_existing_code(void) {
    /*
     * 这个函数展示了如何在现有代码中集成AI推理功能
     * 您可以参考这个模式在您的主程序中添加AI功能
     */
    
    // 检查AI功能是否启用
    if (!camera_ai_is_enabled()) {
        printf("AI功能未启用\n");
        return;
    }
    
    // 进行一次AI推理
    ai_result_t result;
    if (camera_ai_predict_current_frame(&result)) {
        if (result.success && result.meets_threshold) {
            // AI推理成功且置信度足够
            printf("AI预测: %s (置信度: %.3f)\n", result.class_name, result.confidence);
            
            // 在这里添加您的控制逻辑
            switch (result.class_id) {
                case AI_CLASS_LEFT:
                    // 您的左转逻辑
                    break;
                case AI_CLASS_FORWARD:
                    // 您的直行逻辑
                    break;
                case AI_CLASS_RIGHT:
                    // 您的右转逻辑
                    break;
                case AI_CLASS_STOP:
                    // 您的停止逻辑
                    break;
            }
        } else {
            // 置信度不足，执行保守策略
            printf("AI置信度不足，执行保守策略\n");
        }
    } else {
        // AI推理失败，处理错误
        printf("AI推理失败: %s\n", result.error_msg);
    }
}
