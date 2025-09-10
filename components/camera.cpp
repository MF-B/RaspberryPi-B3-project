#include "camera.h"
#include <opencv2/opencv.hpp>
#include <sys/stat.h>

using namespace cv;

// 全局变量
static VideoCapture g_camera;
static camera_state_t g_camera_state = {0};
static bool g_camera_initialized = false;

// 确保输出目录存在
static void ensure_output_directory() {
    struct stat st = {0};
    if (stat("web/static/images", &st) == -1) {
        system("mkdir -p web/static/images");
    }
}

int camera_init(void) {
    printf("初始化摄像头...\n");
    
    ensure_output_directory();
    
    // 尝试打开摄像头
    g_camera.open(0);
    if (!g_camera.isOpened()) {
        printf("错误: 无法打开摄像头\n");
        return -1;
    }
    
    // 设置基本参数
    g_camera.set(CAP_PROP_FRAME_WIDTH, 640);
    g_camera.set(CAP_PROP_FRAME_HEIGHT, 480);
    
    g_camera_initialized = true;
    g_camera_state.status = CAMERA_STATUS_RUNNING;
    g_camera_state.frame_count = 0;
    g_camera_state.stream_running = 0;
    
    printf("摄像头初始化成功\n");
    return 0;
}

int camera_take_snapshot(void) {
    if (!g_camera_initialized) {
        if (camera_init() != 0) {
            return -1;
        }
    }
    
    Mat frame;
    if (!g_camera.read(frame) || frame.empty()) {
        printf("错误: 无法捕获图像\n");
        return -1;
    }
    
    if (imwrite("web/static/images/snapshot.jpg", frame)) {
        g_camera_state.frame_count++;
        printf("快照已保存: web/static/images/snapshot.jpg\n");
        return 0;
    } else {
        printf("错误: 保存图像失败\n");
        return -1;
    }
}

int camera_start_stream(void) {
    if (!g_camera_initialized) {
        if (camera_init() != 0) {
            return -1;
        }
    }
    
    g_camera_state.stream_running = 1;
    printf("视频流已启动\n");
    return 0;
}

int camera_stop_stream(void) {
    g_camera_state.stream_running = 0;
    printf("视频流已停止\n");
    return 0;
}

void camera_cleanup(void) {
    if (g_camera_initialized) {
        g_camera.release();
        g_camera_initialized = false;
    }
    memset(&g_camera_state, 0, sizeof(g_camera_state));
    printf("摄像头资源清理完成\n");
}

camera_state_t camera_get_state(void) {
    return g_camera_state;
}

int camera_is_available(void) {
    return g_camera_initialized && g_camera.isOpened();
}

// 用于Web流的快照更新
int camera_update_stream_snapshot(void) {
    if (!g_camera_state.stream_running || !g_camera_initialized) {
        return -1;
    }
    
    Mat frame;
    if (!g_camera.read(frame) || frame.empty()) {
        return -1;
    }
    
    if (imwrite("web/static/images/stream.jpg", frame)) {
        g_camera_state.frame_count++;
        return 0;
    }
    return -1;
}
