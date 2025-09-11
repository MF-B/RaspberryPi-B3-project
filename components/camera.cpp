#include "camera.h"
#include <opencv2/opencv.hpp>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <mutex>

using namespace cv;

extern "C" {

// 全局变量
static VideoCapture g_camera;
static camera_state_t g_camera_state = {CAMERA_STATUS_STOPPED, 0, 0};
static bool g_camera_initialized = false;
static pthread_t g_stream_thread;
static std::atomic<bool> g_stream_should_stop(false);
static pthread_mutex_t g_camera_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_target_fps = 15; // 目标帧率，降低到15fps提高稳定性
static Mat g_current_frame; // 当前帧缓存

// 确保输出目录存在
static void ensure_output_directory() {
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (stat("web/static/images", &st) == -1) {
        system("mkdir -p web/static/images");
    }
}

// 视频流线程函数
static void* stream_thread_func(void* arg) {
    (void)arg; // 避免未使用参数警告
    
    Mat frame;
    int frame_counter = 0;
    int frame_delay = 1000000 / g_target_fps; // 微秒延迟
    
    printf("视频流线程启动，目标帧率: %d fps\n", g_target_fps);
    
    while (!g_stream_should_stop.load()) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 快速获取帧
        bool frame_captured = false;
        {
            std::lock_guard<std::mutex> lock(*reinterpret_cast<std::mutex*>(&g_camera_mutex));
            if (g_camera_initialized && g_camera.isOpened()) {
                g_camera >> frame;
                if (!frame.empty()) {
                    frame.copyTo(g_current_frame);
                    frame_captured = true;
                    g_camera_state.frame_count++;
                    frame_counter++;
                }
            }
        }
        
        // 在锁外处理图像保存
        if (frame_captured) {
            // 每2帧保存一次，减少磁盘I/O
            if (frame_counter % 2 == 0) {
                std::vector<int> compression_params;
                compression_params.push_back(IMWRITE_JPEG_QUALITY);
                compression_params.push_back(85); // 85%质量，平衡文件大小和质量
                
                if (imwrite("web/static/images/live_frame.jpg", g_current_frame, compression_params)) {
                    // 每60帧打印一次状态信息
                    if (frame_counter % 60 == 0) {
                        printf("视频流运行中，已捕获 %d 帧\n", g_camera_state.frame_count);
                    }
                }
            }
        }
        
        // 精确控制帧率
        auto end_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        int remaining_delay = frame_delay - elapsed.count();
        
        if (remaining_delay > 0) {
            usleep(remaining_delay);
        }
    }
    
    printf("视频流线程结束\n");
    return NULL;
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
    
    // 设置优化参数
    g_camera.set(CAP_PROP_FRAME_WIDTH, 640);
    g_camera.set(CAP_PROP_FRAME_HEIGHT, 480);
    g_camera.set(CAP_PROP_FPS, g_target_fps); // 设置摄像头帧率
    g_camera.set(CAP_PROP_BUFFERSIZE, 1); // 减少缓冲区，降低延迟
    
    // 尝试设置MJPEG编码，可能提高性能
    g_camera.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    
    g_camera_initialized = true;
    g_camera_state.status = CAMERA_STATUS_RUNNING;
    g_camera_state.frame_count = 0;
    g_camera_state.stream_running = 0;
    
    printf("摄像头初始化完成，分辨率: 640x480, 目标帧率: %d fps\n", g_target_fps);
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
    
    // 停止视频流
    camera_stop_stream();
    
    pthread_mutex_lock(&g_camera_mutex);
    if (g_camera_initialized) {
        g_camera.release();
        g_camera_initialized = false;
    }
    pthread_mutex_unlock(&g_camera_mutex);
    
    g_camera_state = {CAMERA_STATUS_STOPPED, 0, 0};
    printf("摄像头组件清理完成\n");
}

// 启动视频流
int camera_start_stream(void) {
    if (!g_camera_initialized) {
        if (camera_init() != 0) {
            return -1;
        }
    }
    
    if (g_camera_state.stream_running) {
        printf("视频流已在运行中\n");
        return 0;
    }
    
    g_stream_should_stop.store(false);
    
    if (pthread_create(&g_stream_thread, NULL, stream_thread_func, NULL) != 0) {
        printf("错误: 无法创建视频流线程\n");
        return -1;
    }
    
    g_camera_state.stream_running = 1;
    printf("视频流已启动\n");
    return 0;
}

// 停止视频流
int camera_stop_stream(void) {
    if (!g_camera_state.stream_running) {
        return 0;
    }
    
    printf("正在停止视频流...\n");
    g_stream_should_stop.store(true);
    
    // 等待线程结束
    if (pthread_join(g_stream_thread, NULL) != 0) {
        printf("警告: 等待视频流线程结束时出错\n");
    }
    
    g_camera_state.stream_running = 0;
    printf("视频流已停止\n");
    return 0;
}

// 捕获单帧并保存到指定文件
int camera_capture_frame_to_file(const char* filename) {
    if (!g_camera_initialized) {
        if (camera_init() != 0) {
            return -1;
        }
    }
    
    pthread_mutex_lock(&g_camera_mutex);
    
    Mat frame;
    g_camera >> frame;
    if (frame.empty()) {
        pthread_mutex_unlock(&g_camera_mutex);
        printf("错误: 无法捕获图像\n");
        return -1;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "web/static/images/%s", filename);
    
    int result = 0;
    if (imwrite(full_path, frame)) {
        g_camera_state.frame_count++;
        printf("图像已保存: %s\n", full_path);
        result = 0;
    } else {
        printf("错误: 保存图像失败 %s\n", full_path);
        result = -1;
    }
    
    pthread_mutex_unlock(&g_camera_mutex);
    return result;
}

// 检查是否正在进行视频流
int camera_is_streaming(void) {
    return g_camera_state.stream_running;
}

// 设置视频流帧率
int camera_set_fps(int fps) {
    if (fps < 1 || fps > 30) {
        printf("错误: 帧率必须在1-30之间\n");
        return -1;
    }
    
    g_target_fps = fps;
    
    if (g_camera_initialized && g_camera.isOpened()) {
        g_camera.set(CAP_PROP_FPS, fps);
    }
    
    printf("目标帧率设置为: %d fps\n", fps);
    return 0;
}

} // extern "C"
