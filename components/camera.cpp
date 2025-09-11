#include "camera.h"
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <string.h>

using namespace cv;

extern "C" {

// 全局变量
static VideoCapture g_camera;
static camera_state_t g_camera_state = {CAMERA_STATUS_STOPPED, 0, 0};
static bool g_camera_initialized = false;

// 确保输出目录存在
static void ensure_output_directory() {
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (stat("web/static/images", &st) == -1) {
        system("mkdir -p web/static/images");
    }
}

// 初始化摄像头
int camera_init(void) {
    printf("摄像头初始化开始...\n");
    
    ensure_output_directory();
    
    // 尝试打开摄像头
    g_camera.open(0);
    if (!g_camera.isOpened()) {
        printf("错误: 无法打开摄像头\n");
        g_camera_state.status = CAMERA_STATUS_ERROR;
        return -1;
    }
    
    // 设置基本参数
    g_camera.set(CAP_PROP_FRAME_WIDTH, 640);
    g_camera.set(CAP_PROP_FRAME_HEIGHT, 480);
    
    g_camera_initialized = true;
    g_camera_state.status = CAMERA_STATUS_RUNNING;
    g_camera_state.frame_count = 0;
    g_camera_state.stream_running = 0;
    
    printf("摄像头初始化完成\n");
    return 0;
}

// 拍摄快照
int camera_take_snapshot(void) {
    if (!g_camera_initialized) {
        if (camera_init() != 0) {
            return -1;
        }
    }
    
    Mat frame;
    g_camera >> frame;
    if (frame.empty()) {
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

// 检查摄像头是否可用
int camera_is_available(void) {
    return g_camera_initialized && g_camera.isOpened();
}

// 获取摄像头状态
camera_state_t camera_get_state(void) {
    return g_camera_state;
}

// 清理摄像头资源
void camera_cleanup(void) {
    printf("摄像头组件清理中...\n");
    
    if (g_camera_initialized) {
        g_camera.release();
        g_camera_initialized = false;
    }
    
    g_camera_state = {CAMERA_STATUS_STOPPED, 0, 0};
    printf("摄像头组件清理完成\n");
}

} // extern "C"
