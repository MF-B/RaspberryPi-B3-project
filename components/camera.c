#include "camera.h"

// 全局状态
static camera_state_t g_camera_state = {0};
static int g_camera_initialized = 0;

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
    
    // 检查摄像头设备
    if (camera_check_device() != 0) {
        printf("警告: 摄像头设备不可用\n");
        g_camera_state.status = CAMERA_STATUS_ERROR;
    }
    
    g_camera_initialized = 1;
    printf("摄像头模块初始化完成\n");
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
    
    g_camera_initialized = 0;
    printf("摄像头资源清理完成\n");
    return 0;
}

// 检查摄像头设备是否可用
int camera_check_device(void) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "test -c %s", g_camera_state.config.device);
    return system(cmd);
}

// 检查摄像头是否可用
int camera_is_available(void) {
    return camera_check_device() == 0;
}

// 拍摄快照
int camera_take_snapshot(void) {
    if (!g_camera_initialized) {
        printf("摄像头未初始化\n");
        return -1;
    }
    
    if (!camera_is_available()) {
        printf("摄像头设备不可用\n");
        g_camera_state.status = CAMERA_STATUS_ERROR;
        return -1;
    }
    
    printf("拍摄快照...\n");
    return camera_capture_frame(CAMERA_SNAPSHOT_PATH);
}

// 捕获单帧图像
int camera_capture_frame(const char* output_path) {
    char cmd[512];
    
    // 使用raspistill命令（如果是树莓派）
    if (access("/usr/bin/raspistill", F_OK) == 0) {
        snprintf(cmd, sizeof(cmd), 
            "raspistill -o %s -w %d -h %d -q %d -t 1 -n > /dev/null 2>&1",
            output_path,
            g_camera_state.config.width,
            g_camera_state.config.height,
            g_camera_state.config.quality
        );
    }
    // 使用fswebcam命令（通用USB摄像头）
    else if (access("/usr/bin/fswebcam", F_OK) == 0) {
        snprintf(cmd, sizeof(cmd),
            "fswebcam -d %s -r %dx%d --jpeg %d --no-banner %s > /dev/null 2>&1",
            g_camera_state.config.device,
            g_camera_state.config.width,
            g_camera_state.config.height,
            g_camera_state.config.quality,
            output_path
        );
    }
    // 使用ffmpeg命令（备选方案）
    else if (access("/usr/bin/ffmpeg", F_OK) == 0) {
        snprintf(cmd, sizeof(cmd),
            "ffmpeg -f v4l2 -i %s -vframes 1 -s %dx%d %s -y > /dev/null 2>&1",
            g_camera_state.config.device,
            g_camera_state.config.width,
            g_camera_state.config.height,
            output_path
        );
    }
    else {
        printf("未找到摄像头捕获工具 (raspistill, fswebcam, ffmpeg)\n");
        return -1;
    }
    
    int result = system(cmd);
    if (result == 0) {
        printf("成功拍摄快照: %s\n", output_path);
        g_camera_state.last_frame_time = time(NULL);
        g_camera_state.frame_count++;
        return 0;
    } else {
        printf("拍摄快照失败\n");
        return -1;
    }
}

// 摄像头流式传输工作线程
void* camera_stream_worker(void* arg) {
    (void)arg; // 避免未使用参数警告
    
    printf("摄像头流式传输线程启动\n");
    
    while (g_camera_state.stream_running) {
        if (camera_capture_frame(CAMERA_STREAM_PATH) == 0) {
            g_camera_state.status = CAMERA_STATUS_RUNNING;
        } else {
            g_camera_state.status = CAMERA_STATUS_ERROR;
            printf("流式传输出错，停止线程\n");
            break;
        }
        
        // 根据FPS设置延迟
        usleep(1000000 / g_camera_state.config.fps);
    }
    
    printf("摄像头流式传输线程结束\n");
    return NULL;
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
    
    if (!camera_is_available()) {
        printf("摄像头设备不可用\n");
        g_camera_state.status = CAMERA_STATUS_ERROR;
        return -1;
    }
    
    printf("启动摄像头流式传输...\n");
    
    g_camera_state.stream_running = 1;
    
    // 创建流式传输线程
    if (pthread_create(&g_camera_state.stream_thread, NULL, camera_stream_worker, NULL) != 0) {
        printf("创建流式传输线程失败\n");
        g_camera_state.stream_running = 0;
        return -1;
    }
    
    printf("摄像头流式传输已启动\n");
    return 0;
}

// 停止流式传输
int camera_stop_stream(void) {
    if (!g_camera_state.stream_running) {
        return 0;
    }
    
    printf("停止摄像头流式传输...\n");
    
    g_camera_state.stream_running = 0;
    
    // 等待线程结束
    if (pthread_join(g_camera_state.stream_thread, NULL) != 0) {
        printf("等待流式传输线程结束失败\n");
    }
    
    g_camera_state.status = CAMERA_STATUS_STOPPED;
    printf("摄像头流式传输已停止\n");
    return 0;
}

// 获取摄像头状态
camera_state_t camera_get_state(void) {
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
