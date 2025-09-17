/*
 * AI推理功能测试程序
 * 测试摄像头AI推理和车轮控制的集成功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <wiringPi.h>
#include "components/camera.h"
#include "components/control.h"
#include "ai_wrapper.h"

// 测试配置
#define TEST_DURATION_SECONDS 30
#define INFERENCE_INTERVAL_MS 500  // 500ms推理一次
#define CONTROL_SPEED 30           // 车轮控制速度

// 根据AI推理结果控制车轮
void control_wheels_by_ai_result(const ai_result_t* result) {
    if (!result || !result->success) {
        // AI推理失败，停止车轮
        printf("AI推理失败，停止车轮\n");
        wheel_stop();
        return;
    }
    
    if (!result->meets_threshold) {
        // 置信度不足，停止车轮
        printf("置信度不足 (%.3f)，停止车轮\n", result->confidence);
        wheel_stop();
        return;
    }
    
    // 根据AI预测类别控制车轮
    printf("AI预测: %s (置信度: %.3f)\n", result->class_name, result->confidence);
    
    switch (result->class_id) {
        case AI_CLASS_LEFT:
            printf("执行左转\n");
            wheel_left(CONTROL_SPEED);
            break;
            
        case AI_CLASS_FORWARD:
            printf("执行直行\n");
            wheel_forward(CONTROL_SPEED);
            break;
            
        case AI_CLASS_RIGHT:
            printf("执行右转\n");
            wheel_right(CONTROL_SPEED);
            break;
            
        case AI_CLASS_STOP:
        default:
            printf("执行停止\n");
            wheel_stop();
            break;
    }
}

// 主测试函数
int main(int argc, char* argv[]) {
    printf("=== AI推理功能测试程序 ===\n");
    
    // 解析命令行参数
    const char* model_path = (argc > 1) ? argv[1] : "model/line_follower.tflite";
    
    // 初始化wiringPi
    if (wiringPiSetup() == -1) {
        printf("错误: wiringPi初始化失败\n");
        return 1;
    }
    
    // 初始化车轮控制
    printf("初始化车轮控制...\n");
    wheel_init();
    
    // 初始化摄像头
    printf("初始化摄像头...\n");
    if (camera_init()!=0) {
        printf("错误: 摄像头初始化失败\n");
        return 1;
    }
    
    // 初始化AI推理功能
    printf("初始化AI推理功能 (模型: %s)...\n", model_path);
    if (!camera_ai_init(model_path)) {
        printf("错误: AI推理功能初始化失败\n");
        camera_cleanup();
        return 1;
    }
    
    printf("所有模块初始化完成，开始AI推理测试...\n");
    printf("测试时长: %d秒\n", TEST_DURATION_SECONDS);
    printf("推理间隔: %dms\n", INFERENCE_INTERVAL_MS);
    printf("按Ctrl+C停止测试\n\n");
    
    // 主测试循环
    int test_count = 0;
    int success_count = 0;
    time_t start_time = time(NULL);
    
    while (time(NULL) - start_time < TEST_DURATION_SECONDS) {
        ai_result_t result;
        
        printf("--- 第%d次推理 ---\n", ++test_count);
        
        // 进行AI推理
        if (camera_ai_predict_current_frame(&result)) {
            success_count++;
            ai_print_result(&result);
            
            // 根据推理结果控制车轮
            control_wheels_by_ai_result(&result);
        } else {
            printf("AI推理失败: %s\n", result.error_msg);
            wheel_stop();  // 推理失败时停止车轮
        }
        
        printf("\n");
        
        // 等待下次推理
        delay(INFERENCE_INTERVAL_MS);
    }
    
    // 测试结束，停止所有运动
    printf("=== 测试结束 ===\n");
    wheel_stop();
    
    // 打印测试统计
    printf("测试统计:\n");
    printf("  总推理次数: %d\n", test_count);
    printf("  成功次数: %d\n", success_count);
    printf("  成功率: %.1f%%\n", test_count > 0 ? (success_count * 100.0 / test_count) : 0.0);
    
    // 清理资源
    printf("清理资源...\n");
    camera_ai_cleanup();
    camera_cleanup();
    
    printf("测试程序完成\n");
    return 0;
}

// 简单的AI推理测试 (不涉及车轮控制)
int test_ai_inference_only(void) {
    printf("=== 仅AI推理测试 ===\n");
    
    // 初始化AI模块
    if (!ai_wrapper_init("model/line_follower.tflite")) {
        printf("AI模块初始化失败\n");
        return 1;
    }
    
    // 创建测试图像 (模拟线路图像)
    unsigned char test_image[224 * 224 * 3];  // 假设模型输入为224x224
    memset(test_image, 128, sizeof(test_image));  // 灰色背景
    
    // 绘制白色线条 (模拟线路)
    for (int y = 0; y < 224; y++) {
        for (int x = 110; x < 114; x++) {  // 中央线条
            int idx = (y * 224 + x) * 3;
            test_image[idx] = 255;     // B
            test_image[idx+1] = 255;   // G  
            test_image[idx+2] = 255;   // R
        }
    }
    
    // 进行推理测试
    ai_result_t result;
    if (ai_wrapper_predict(test_image, 224, 224, 0.5, &result)) {
        printf("AI推理测试成功:\n");
        ai_print_result(&result);
    } else {
        printf("AI推理测试失败: %s\n", result.error_msg);
    }
    
    // 清理
    ai_wrapper_cleanup();
    return result.success ? 0 : 1;
}
