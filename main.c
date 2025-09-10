#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <wiringPi.h>
#include "web/http_server.h"
#include "components/beep.h"
#include "components/button.h"
#include "components/clock.h"
#include "components/rgb.h"
#include "components/temp.h"
#include "components/distance.h"
#include "components/control.h"  // 新增运动控制
#include "components/camera.h"   // 新增摄像头

// 函数声明
void show_web_menu(void);
void *web_server_thread(void *arg);
void init_all_components(void);
void cleanup_all_components(void);

int main(void) {
    int choice;
    
    printf("=== Web服务器启动 ===\n");
    printf("检查用户权限: UID=%d, GID=%d\n", getuid(), getgid());
    
    // 初始化 wiringPi
    printf("正在初始化 wiringPi...\n");
    if (wiringPiSetupGpio() == -1) {
        printf("初始化 wiringPi 失败!\n");
        return 1;
    }
    printf("wiringPi 初始化成功!\n");
    
    printf("=== 树莓派B3项目控制系统 ===\n");
    printf("系统初始化中...\n");
    
    // 初始化所有组件
    init_all_components();
    
    printf("系统初始化完成!\n");
    delay(1000);
    
    while (1) {
        system("clear");
        show_web_menu();
        
        printf("请选择功能 (1-4): ");
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: {
                // 启动Web服务器
                printf("\n正在启动Web服务器...\n");
                
                // 设置信号处理
                setup_signal_handlers();
                
                // 在主线程中运行服务器
                printf("Web服务器启动中，请访问: http://localhost:8080\n");
                printf("按Ctrl+C停止服务器并返回菜单\n\n");
                
                if (start_server(SERVER_PORT) == 0) {
                    printf("Web服务器已停止\n");
                } else {
                    printf("Web服务器启动失败\n");
                }
                
                printf("按回车键继续...");
                getchar();
                getchar();
                break;
            }
            case 2: {
                // 显示API文档
                system("clear");
                printf("\n=== API接口文档 ===\n\n");
                printf("基础URL: http://localhost:8080/api/\n\n");
                
                printf("可用的API端点:\n");
                printf("1. GET  /api/status     - 获取系统状态\n");
                printf("2. GET  /api/sensors    - 获取传感器数据\n");
                printf("3. POST /api/rgb        - 控制RGB LED\n");
                printf("4. POST /api/beep       - 控制蜂鸣器\n");
                printf("5. GET  /api/distance   - 获取距离数据\n");
                printf("6. POST /api/camera     - 控制摄像头\n\n");
                
                printf("使用示例:\n");
                printf("curl -X GET http://localhost:8080/api/status\n");
                printf("curl -X POST -H \"Content-Type: application/json\" \\\n");
                printf("     -d '{\"action\":\"on\",\"color\":\"red\"}' \\\n");
                printf("     http://localhost:8080/api/rgb\n\n");
                
                printf("Web界面: http://localhost:8080/\n\n");
                
                printf("按回车键继续...");
                getchar();
                getchar();
                break;
            }
            case 3: {
                // 运行原有的命令行模式
                system("clear");
                printf("正在启动传统命令行模式...\n");
                printf("注意: 这将退出当前程序并运行原始main.c\n");
                printf("如需返回Web模式，请重新运行 ./web_main\n");
                
                // 清理资源
                cleanup_all_components();
                
                // 运行原始程序
                system("./main");
                return 0;
            }
            case 4:
                printf("感谢使用！再见！\n");
                cleanup_all_components();
                exit(0);
                break;
            default:
                printf("无效选择，请重新输入！\n");
                printf("按回车键继续...");
                getchar();
                getchar();
                break;
        }
    }
    
    return 0;
}

void show_web_menu(void) {
    printf("\n");
    printf("╔══════════════════════════════════════╗\n");
    printf("║       树莓派B3 Web控制系统          ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║  1. 启动Web服务器                    ║\n");
    printf("║  2. 查看API文档                      ║\n");
    printf("║  3. 切换到命令行模式                 ║\n");
    printf("║  4. 退出系统                         ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("\n");
    printf("Web界面功能:\n");
    printf("• 实时传感器数据显示 (温度、湿度、距离)\n");
    printf("• RGB LED颜色控制\n");
    printf("• 蜂鸣器音响控制\n");
    printf("• 舵机角度控制\n");
    printf("• 系统状态监控\n");
    printf("• 响应式设计，支持手机访问\n");
    printf("\n");
}

void init_all_components(void) {
    printf("开始初始化硬件组件...\n");
    
    // 初始化各个组件
    printf("1. 初始化蜂鸣器...");
    beep_init();
    printf(" 完成\n");
    
    printf("2. 初始化按钮...");
    button_init();
    printf(" 完成\n");
    
    printf("3. 初始化时钟显示...");
    clock_init();
    printf(" 完成\n");
    
    printf("4. 初始化RGB LED...");
    rgb_init();
    printf(" 完成\n");
    
    printf("5. 初始化距离传感器...");
    distance_init();
    printf(" 完成\n");
    
    printf("6. 初始化温度传感器...");
    temp_init();
    printf(" 完成\n");
    
    printf("7. 初始化运动控制...");
    init_wheel();  // 新增运动控制初始化
    printf(" 完成\n");
    
    printf("8. 启动摄像头服务...");
    // 启动摄像头守护进程
    system("./camera_service.sh start > /dev/null 2>&1");
    // 等待服务启动
    delay(2000);
    camera_init();   // 初始化摄像头模块
    printf(" 完成\n");
    
    printf("所有硬件组件初始化完成\n");
}

void cleanup_all_components(void) {
    // 清理资源
    beep_off();
    rgb_cleanup();
    clock_cleanup();
    clean_wheel();  // 新增运动控制清理
    camera_cleanup();   // 新增摄像头清理
    // 停止摄像头守护进程
    system("./camera_service.sh stop > /dev/null 2>&1");
    
    printf("硬件资源清理完成\n");
}
