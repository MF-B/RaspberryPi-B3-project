#include "camera.h"

// 全局状态
static camera_state_t g_camera_state = {0};
static int g_camera_initialized = 0;

// 执行Python摄像头脚本并解析结果
static int execute_camera_command(const char* command, char* result_buffer, size_t buffer_size) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cd /home/mf1bzz/Code/raspberry/RaspberryPi-B3-project && python3 components/camera.py %s 2>/dev/null", command);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        printf("执行摄像头命令失败: %s\n", command);
        return -1;
    }
    
    // 读取命令输出
    if (fgets(result_buffer, buffer_size, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    
    int exit_code = pclose(fp);
    return exit_code;
}

// 解析JSON响应（简单的JSON解析）
static int parse_success_from_json(const char* json_str) {
    // 简单查找 "success": true
    if (strstr(json_str, "\"success\": true") || strstr(json_str, "\"success\":true")) {
        return 1;
    }
    return 0;
}

// 初始化摄像头
int camera_init(void) {
    if (g_camera_initialized) {
        return 0; // 已经初始化
    }
    
    printf("初始化摄像头模块...\n");
    
    // 初始化状态
    memset(&g_camera_state, 0, sizeof(g_camera_state));
    g_camera_state.status = CAMERA_STATUS_STOPPED;
    g_camera_state.stream_running = 0;
    
    // 设置默认配置
    g_camera_state.config.width = CAMERA_WIDTH;
    g_camera_state.config.height = CAMERA_HEIGHT;
    g_camera_state.config.fps = CAMERA_FPS;
    g_camera_state.config.quality = CAMERA_QUALITY;
    strcpy(g_camera_state.config.device, CAMERA_DEVICE);
    g_camera_state.config.auto_exposure = 1;
    g_camera_state.config.brightness = 50;
    g_camera_state.config.contrast = 50;
    
    // 创建图片存储目录
    system("mkdir -p web/static/images");
    
    // 初始化Python摄像头
    char result[1024];
    if (execute_camera_command("init", result, sizeof(result)) == 0) {
        if (parse_success_from_json(result)) {
            printf("摄像头模块初始化成功\n");
            g_camera_state.status = CAMERA_STATUS_STOPPED;
        } else {
            printf("摄像头初始化失败\n");
            g_camera_state.status = CAMERA_STATUS_ERROR;
        }
    } else {
        printf("摄像头命令执行失败\n");
        g_camera_state.status = CAMERA_STATUS_ERROR;
    }
    
    g_camera_initialized = 1;
    return 0;
}

// 清理摄像头资源
int camera_cleanup(void) {
    if (!g_camera_initialized) {
        return 0;
    }
    
    printf("清理摄像头资源...\n");
    
    // 停止流式传输
    camera_stop_stream();
    
    // 调用Python清理命令
    char result[1024];
    execute_camera_command("cleanup", result, sizeof(result));
    
    g_camera_initialized = 0;
    printf("摄像头资源清理完成\n");
    return 0;
}

// 检查摄像头是否可用
int camera_is_available(void) {
    char result[1024];
    if (execute_camera_command("status", result, sizeof(result)) == 0) {
        // 检查JSON中的available字段
        if (strstr(result, "\"available\": true") || strstr(result, "\"available\":true")) {
            return 1;
        }
    }
    return 0;
}

// 拍摄快照
int camera_take_snapshot(void) {
    if (!g_camera_initialized) {
        printf("摄像头未初始化\n");
        return -1;
    }
    
    printf("拍摄快照...\n");
    
    char result[1024];
    if (execute_camera_command("snapshot", result, sizeof(result)) == 0) {
        if (parse_success_from_json(result)) {
            printf("快照拍摄成功\n");
            g_camera_state.last_frame_time = time(NULL);
            g_camera_state.frame_count++;
            return 0;
        }
    }
    
    printf("快照拍摄失败\n");
    return -1;
}

// 开始流式传输
int camera_start_stream(void) {
    if (!g_camera_initialized) {
        printf("摄像头未初始化\n");
        return -1;
    }
    
    if (g_camera_state.stream_running) {
        printf("流式传输已在运行\n");
        return 0;
    }
    
    printf("启动摄像头流式传输...\n");
    
    char result[1024];
    if (execute_camera_command("start_stream", result, sizeof(result)) == 0) {
        if (parse_success_from_json(result)) {
            printf("摄像头流式传输已启动\n");
            g_camera_state.stream_running = 1;
            g_camera_state.status = CAMERA_STATUS_RUNNING;
            return 0;
        }
    }
    
    printf("启动流式传输失败\n");
    return -1;
}

// 停止流式传输
int camera_stop_stream(void) {
    if (!g_camera_state.stream_running) {
        return 0;
    }
    
    printf("停止摄像头流式传输...\n");
    
    char result[1024];
    execute_camera_command("stop_stream", result, sizeof(result));
    
    g_camera_state.stream_running = 0;
    g_camera_state.status = CAMERA_STATUS_STOPPED;
    printf("摄像头流式传输已停止\n");
    return 0;
}

// 获取摄像头状态
camera_state_t camera_get_state(void) {
    // 更新状态信息
    char result[1024];
    if (execute_camera_command("status", result, sizeof(result)) == 0) {
        // 简单解析JSON状态
        if (strstr(result, "\"streaming\": true") || strstr(result, "\"streaming\":true")) {
            g_camera_state.stream_running = 1;
            g_camera_state.status = CAMERA_STATUS_RUNNING;
        } else {
            g_camera_state.stream_running = 0;
            g_camera_state.status = CAMERA_STATUS_STOPPED;
        }
        
        // 尝试解析frame_count
        char *frame_count_pos = strstr(result, "\"frame_count\":");
        if (frame_count_pos) {
            sscanf(frame_count_pos + 14, "%d", &g_camera_state.frame_count);
        }
    }
    
    return g_camera_state;
}

// 设置摄像头配置
int camera_set_config(camera_config_t *config) {
    if (!g_camera_initialized || !config) {
        return -1;
    }
    
    // 如果正在流式传输，需要重启
    int was_streaming = g_camera_state.stream_running;
    if (was_streaming) {
        camera_stop_stream();
    }
    
    // 更新配置
    g_camera_state.config = *config;
    
    // 重新启动流式传输
    if (was_streaming) {
        camera_start_stream();
    }
    
    return 0;
}
