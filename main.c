/**
 * @file main.c
 * @brief 树莓派B3项目Web控制系统主程序
 * @details 自动初始化硬件组件并启动Web服务器，提供硬件控制的Web界面
 * @author MF-B
 * @date 2025-09-10
 * @version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>
#include "log.h" // log.c 日志库
#include "web/http_server.h"
#include "components/beep.h"
#include "components/button.h"
#include "components/clock.h"
#include "components/rgb.h"
#include "components/temp.h"
#include "components/distance.h"
#include "components/control.h" // 运动控制
#include "components/camera.h"  // 摄像头

// 函数声明
int init_all_components(void);
void cleanup_all_components(void);

/**
 * @brief 主函数 - 直接启动Web服务器
 * @details 初始化系统组件后直接启动Web服务器，无需用户交互
 * @return 0表示正常退出，其他值表示错误
 */
int main(void)
{
    // 初始化日志系统
    log_set_level(LOG_INFO);
    log_info("========================================");
    log_info("  树莓派B3项目 Web控制系统 v2.0");
    log_info("========================================");
    log_info("启动时间: %s", __DATE__ " " __TIME__);
    log_info("检查用户权限: UID=%d, GID=%d", getuid(), getgid());

    // 初始化 wiringPi
    log_info("正在初始化 wiringPi GPIO库...");
    if (wiringPiSetupGpio() == -1)
    {
        log_error("初始化 wiringPi 失败!");
        log_error("请确保:");
        log_error("  1. 以root权限运行程序: sudo ./program");
        log_error("  2. 当前系统支持wiringPi库");
        log_error("  3. GPIO设备可访问");
        return 1;
    }
    log_info("wiringPi GPIO库初始化成功!");

    log_info("开始系统组件初始化...");

    // 初始化所有组件
    if (init_all_components() != 0)
    {
        log_error("组件初始化失败！程序退出");
        return 1;
    }

    log_info("所有组件初始化完成!");
    log_info("准备启动Web服务器...");

    // 设置信号处理以确保正确清理资源
    log_info("设置信号处理器...");
    setup_signal_handlers();

    // 显示服务信息
    log_info("========================================");
    log_info("         Web服务器启动成功!");
    log_info("========================================");
    log_info("访问地址: http://localhost:8080");
    log_info("本地网络: http://<树莓派IP>:8080");
    log_info("Web界面功能:");
    log_info("• 实时传感器数据监控");
    log_info("• RGB LED颜色控制");
    log_info("• 蜂鸣器音响控制");
    log_info("• 运动控制系统");
    log_info("• 摄像头控制");
    log_info("• 系统状态监控");
    log_info("API接口: http://localhost:8080/api/");
    log_info("使用 Ctrl+C 停止服务器");
    log_info("========================================");

    // 启动Web服务器（阻塞运行）
    log_info("启动HTTP服务器监听端口 %d...", SERVER_PORT);
    int server_result = start_server(SERVER_PORT);

    if (server_result == 0)
    {
        log_info("Web服务器正常停止");
    }
    else
    {
        log_error("Web服务器异常退出，错误代码: %d", server_result);
    }

    // 清理资源
    log_info("开始清理系统资源...");
    cleanup_all_components();
    log_info("程序退出");

    return server_result;
}

/**
 * @brief 初始化所有硬件组件
 * @return 0表示成功，-1表示失败
 */
int init_all_components(void)
{
    log_info("=== 硬件组件初始化开始 ===");

    // 初始化各个组件
    log_info("[1/8] 初始化蜂鸣器模块...");
    beep_init();
    log_info("蜂鸣器模块初始化成功");

    log_info("[2/8] 初始化按钮模块...");
    button_init();
    log_info("按钮模块初始化成功");

    log_info("[3/8] 初始化时钟显示模块...");
    clock_init();
    log_info("时钟显示模块初始化成功");

    log_info("[4/8] 初始化RGB LED模块...");
    rgb_init();
    log_info("RGB LED模块初始化成功");

    log_info("[5/8] 初始化距离传感器模块...");
    distance_init();
    log_info("距离传感器模块初始化成功");

    log_info("[6/8] 初始化温度传感器模块...");
    temp_init();
    log_info("温度传感器模块初始化成功");

    log_info("[7/8] 初始化运动控制模块...");
    init_wheel(); // 运动控制初始化
    log_info("运动控制模块初始化成功");

    log_info("[8/8] 启动摄像头服务...");
    // 启动摄像头守护进程
    int camera_result = camera_init();
    if (camera_result != 0)
    {
        log_warn("摄像头服务启动失败 (返回码: %d)", camera_result);
        log_warn("摄像头功能可能不可用，但系统将继续运行");
    }
    else
    {
        delay(2000);
        log_info("摄像头模块初始化成功");
    }

    log_info("=== 所有硬件组件初始化完成 ===");
    log_info("组件状态: 蜂鸣器✓ 按钮✓ 时钟✓ RGB✓ 距离✓ 温度✓ 运动✓ 摄像头%s",
             camera_result == 0 ? "✓" : "⚠");

    return 0;
}

/**
 * @brief 清理所有硬件组件资源
 * @details 在程序退出前调用，确保所有硬件组件正确关闭和清理
 */
void cleanup_all_components(void)
{
    log_info("=== 开始清理硬件资源 ===");

    log_info("关闭蜂鸣器...");
    beep_off();
    log_info("蜂鸣器关闭完成");

    log_info("清理RGB LED...");
    rgb_cleanup();
    log_info("RGB LED清理完成");

    log_info("清理时钟显示...");
    clock_cleanup();
    log_info("时钟显示清理完成");

    log_info("清理运动控制模块...");
    clean_wheel();
    log_info("运动控制模块清理完成");

    log_info("清理摄像头模块...");
    camera_cleanup();
    log_info("摄像头模块清理完成");

    log_info("停止摄像头守护进程...");
    log_info("=== 硬件资源清理完成 ===");
}
