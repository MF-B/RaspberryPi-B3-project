#include "camera.h"
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <unistd.h>

using namespace cv;

extern "C" {

// 全局变量
static VideoCapture g_camera;
static camera_state_t g_camera_state = {CAMERA_STATUS_STOPPED, 0, 0};
static bool g_camera_initialized = false;

// MJPEG流相关变量
static pthread_t g_stream_thread;
static volatile bool g_stream_running = false;
static std::queue<mjpeg_frame_t> g_frame_buffer;
static std::mutex g_frame_mutex;
static std::condition_variable g_frame_cv;
static const size_t MAX_FRAME_BUFFER_SIZE = 5;

// MJPEG流线程函数
static void* stream_thread_func(void* arg) {
    Mat frame;
    std::vector<uchar> buffer;
    std::vector<int> params = {IMWRITE_JPEG_QUALITY, 85};
    
    while (g_stream_running) {
        if (!g_camera.isOpened()) {
            usleep(100000); // 100ms
            continue;
        }
        
        g_camera >> frame;
        if (frame.empty()) {
            usleep(33000); // ~30fps
            continue;
        }
        
        // 编码为JPEG
        buffer.clear();
        if (!imencode(".jpg", frame, buffer, params)) {
            usleep(33000);
            continue;
        }
        
        // 创建帧数据
        mjpeg_frame_t mjpeg_frame;
        mjpeg_frame.size = buffer.size();
        mjpeg_frame.width = frame.cols;
        mjpeg_frame.height = frame.rows;
        mjpeg_frame.data = (unsigned char*)malloc(mjpeg_frame.size);
        if (mjpeg_frame.data) {
            memcpy(mjpeg_frame.data, buffer.data(), mjpeg_frame.size);
            
            // 添加到缓冲区
            std::unique_lock<std::mutex> lock(g_frame_mutex);
            
            // 如果缓冲区满了，删除最老的帧
            while (g_frame_buffer.size() >= MAX_FRAME_BUFFER_SIZE) {
                mjpeg_frame_t old_frame = g_frame_buffer.front();
                g_frame_buffer.pop();
                free(old_frame.data);
            }
            
            g_frame_buffer.push(mjpeg_frame);
            g_frame_cv.notify_one();
            
            g_camera_state.frame_count++;
        }
        
        usleep(33000); // ~30fps
    }
    
    return NULL;
}

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
    
    // 停止流
    camera_stop_stream();
    
    if (g_camera_initialized) {
        g_camera.release();
        g_camera_initialized = false;
    }
    
    g_camera_state = {CAMERA_STATUS_STOPPED, 0, 0};
    printf("摄像头组件清理完成\n");
}

// 启动MJPEG视频流
int camera_start_stream(void) {
    if (!g_camera_initialized) {
        if (camera_init() != 0) {
            return -1;
        }
    }
    
    if (g_stream_running) {
        printf("视频流已经在运行中\n");
        return 0;
    }
    
    printf("启动MJPEG视频流...\n");
    g_stream_running = true;
    
    if (pthread_create(&g_stream_thread, NULL, stream_thread_func, NULL) != 0) {
        printf("错误: 无法创建流线程\n");
        g_stream_running = false;
        return -1;
    }
    
    g_camera_state.stream_running = 1;
    printf("MJPEG视频流已启动\n");
    return 0;
}

// 停止MJPEG视频流
int camera_stop_stream(void) {
    if (!g_stream_running) {
        return 0;
    }
    
    printf("停止MJPEG视频流...\n");
    g_stream_running = false;
    
    // 等待线程结束
    pthread_join(g_stream_thread, NULL);
    
    // 清空帧缓冲区
    std::unique_lock<std::mutex> lock(g_frame_mutex);
    while (!g_frame_buffer.empty()) {
        mjpeg_frame_t frame = g_frame_buffer.front();
        g_frame_buffer.pop();
        free(frame.data);
    }
    
    g_camera_state.stream_running = 0;
    printf("MJPEG视频流已停止\n");
    return 0;
}

// 获取MJPEG帧
int camera_get_mjpeg_frame(mjpeg_frame_t *frame) {
    if (!g_stream_running || !frame) {
        return -1;
    }
    
    std::unique_lock<std::mutex> lock(g_frame_mutex);
    
    // 等待新帧（最多等待1秒）
    if (g_frame_buffer.empty()) {
        if (g_frame_cv.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout) {
            return -1; // 超时
        }
    }
    
    if (g_frame_buffer.empty()) {
        return -1;
    }
    
    *frame = g_frame_buffer.front();
    g_frame_buffer.pop();
    
    return 0;
}

// 释放帧内存
void camera_free_frame(mjpeg_frame_t *frame) {
    if (frame && frame->data) {
        free(frame->data);
        frame->data = NULL;
        frame->size = 0;
    }
}

// 检查是否正在流式传输
int camera_is_streaming(void) {
    return g_stream_running ? 1 : 0;
}

} // extern "C"
