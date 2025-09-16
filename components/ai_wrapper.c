#include "ai.h"
#include <python3.11/Python.h>
#include <signal.h>
#include <sys/wait.h>

// 全局变量
static ai_status_t current_status = {AI_STATE_STOPPED, 0, AI_DIRECTION_STOP, 0.0, 0, 0};
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
static int debug_mode = 1; // 启用调试模式
static char model_path[256] = "./model/line_follower.tflite";
static int camera_index = 0;
static pid_t python_pid = -1;
static FILE* python_pipe = NULL;

// Python脚本路径
static const char* AI_SCRIPT_PATH = "./components/ai.py";

// 内部函数声明
static int start_python_process(void);
static int stop_python_process(void);
static int send_python_command(const char* command);
static int read_python_status(void);
static void update_status(ai_state_t state, int enabled, ai_direction_t direction, float confidence);

// 初始化AI模块
int ai_init(void)
{
    if (debug_mode) {
        printf("[AI] 初始化AI模块\n");
    }
    
    // 检查Python脚本是否存在
    if (access(AI_SCRIPT_PATH, F_OK) != 0) {
        printf("[AI] 错误: AI脚本文件不存在: %s\n", AI_SCRIPT_PATH);
        return -1;
    }
    
    // 初始化Python解释器
    if (!Py_IsInitialized()) {
        Py_Initialize();
        if (!Py_IsInitialized()) {
            printf("[AI] 错误: Python解释器初始化失败\n");
            return -1;
        }
    }
    
    // 初始化状态
    pthread_mutex_lock(&status_mutex);
    current_status.state = AI_STATE_STOPPED;
    current_status.is_enabled = 0;
    current_status.current_direction = AI_DIRECTION_STOP;
    current_status.confidence = 0.0;
    current_status.frame_count = 0;
    current_status.last_update = time(NULL);
    pthread_mutex_unlock(&status_mutex);
    
    if (debug_mode) {
        printf("[AI] AI模块初始化完成\n");
    }
    
    return 0;
}

// 清理AI模块
void ai_cleanup(void)
{
    if (debug_mode) {
        printf("[AI] 清理AI模块\n");
    }
    
    // 停止Python进程
    stop_python_process();
    
    // 更新状态
    update_status(AI_STATE_STOPPED, 0, AI_DIRECTION_STOP, 0.0);
    
    // 清理Python解释器
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
    
    if (debug_mode) {
        printf("[AI] AI模块清理完成\n");
    }
}

// 启动AI模块
int ai_start(void)
{
    if (debug_mode) {
        printf("[AI] 启动AI模块\n");
    }
    
    pthread_mutex_lock(&status_mutex);
    if (current_status.state == AI_STATE_RUNNING) {
        pthread_mutex_unlock(&status_mutex);
        if (debug_mode) {
            printf("[AI] AI模块已经在运行\n");
        }
        return 0;
    }
    pthread_mutex_unlock(&status_mutex);
    
    // 启动Python进程
    if (start_python_process() != 0) {
        update_status(AI_STATE_ERROR, 0, AI_DIRECTION_STOP, 0.0);
        return -1;
    }
    
    update_status(AI_STATE_RUNNING, 0, AI_DIRECTION_STOP, 0.0);
    
    if (debug_mode) {
        printf("[AI] AI模块启动成功\n");
    }
    
    return 0;
}

// 停止AI模块
int ai_stop(void)
{
    if (debug_mode) {
        printf("[AI] 停止AI模块\n");
    }
    
    // 禁用AI寻迹
    ai_disable();
    
    // 停止Python进程
    stop_python_process();
    
    update_status(AI_STATE_STOPPED, 0, AI_DIRECTION_STOP, 0.0);
    
    if (debug_mode) {
        printf("[AI] AI模块已停止\n");
    }
    
    return 0;
}

// 启用AI寻迹
int ai_enable(void)
{
    if (debug_mode) {
        printf("[AI] 启用AI寻迹\n");
    }
    
    pthread_mutex_lock(&status_mutex);
    if (current_status.state != AI_STATE_RUNNING) {
        pthread_mutex_unlock(&status_mutex);
        printf("[AI] 错误: AI模块未运行\n");
        return -1;
    }
    pthread_mutex_unlock(&status_mutex);
    
    // 发送启用命令
    if (send_python_command("enable") != 0) {
        return -1;
    }
    
    pthread_mutex_lock(&status_mutex);
    current_status.is_enabled = 1;
    current_status.last_update = time(NULL);
    pthread_mutex_unlock(&status_mutex);
    
    return 0;
}

// 禁用AI寻迹
int ai_disable(void)
{
    if (debug_mode) {
        printf("[AI] 禁用AI寻迹\n");
    }
    
    // 发送禁用命令
    send_python_command("disable");
    
    pthread_mutex_lock(&status_mutex);
    current_status.is_enabled = 0;
    current_status.current_direction = AI_DIRECTION_STOP;
    current_status.confidence = 0.0;
    current_status.last_update = time(NULL);
    pthread_mutex_unlock(&status_mutex);
    
    return 0;
}

// 获取AI状态
ai_status_t ai_get_status(void)
{
    ai_status_t status;
    
    pthread_mutex_lock(&status_mutex);
    status = current_status;
    pthread_mutex_unlock(&status_mutex);
    
    // 尝试从Python进程读取最新状态
    read_python_status();
    
    return status;
}

// 检查AI是否运行
int ai_is_running(void)
{
    pthread_mutex_lock(&status_mutex);
    int running = (current_status.state == AI_STATE_RUNNING);
    pthread_mutex_unlock(&status_mutex);
    
    return running;
}

// 检查AI是否启用
int ai_is_enabled(void)
{
    pthread_mutex_lock(&status_mutex);
    int enabled = current_status.is_enabled;
    pthread_mutex_unlock(&status_mutex);
    
    return enabled;
}

// 获取当前方向
ai_direction_t ai_get_direction(void)
{
    pthread_mutex_lock(&status_mutex);
    ai_direction_t direction = current_status.current_direction;
    pthread_mutex_unlock(&status_mutex);
    
    return direction;
}

// 获取置信度
float ai_get_confidence(void)
{
    pthread_mutex_lock(&status_mutex);
    float confidence = current_status.confidence;
    pthread_mutex_unlock(&status_mutex);
    
    return confidence;
}

// 设置模型路径
int ai_set_model_path(const char* path)
{
    if (!path || strlen(path) >= sizeof(model_path)) {
        return -1;
    }
    
    strcpy(model_path, path);
    return 0;
}

// 设置摄像头索引
int ai_set_camera_index(int index)
{
    if (index < 0) {
        return -1;
    }
    
    camera_index = index;
    return 0;
}

// 设置调试模式
void ai_set_debug_mode(int enabled)
{
    debug_mode = enabled;
}

// 打印状态信息
void ai_print_status(void)
{
    ai_status_t status = ai_get_status();
    
    const char* state_str;
    switch (status.state) {
        case AI_STATE_STOPPED: state_str = "已停止"; break;
        case AI_STATE_RUNNING: state_str = "运行中"; break;
        case AI_STATE_ERROR: state_str = "错误"; break;
        default: state_str = "未知"; break;
    }
    
    const char* direction_str;
    switch (status.current_direction) {
        case AI_DIRECTION_STOP: direction_str = "停止"; break;
        case AI_DIRECTION_FORWARD: direction_str = "直行"; break;
        case AI_DIRECTION_LEFT: direction_str = "左转"; break;
        case AI_DIRECTION_RIGHT: direction_str = "右转"; break;
        default: direction_str = "未知"; break;
    }
    
    printf("[AI状态] 状态: %s, 启用: %s, 方向: %s, 置信度: %.3f, 帧数: %d\n",
           state_str,
           status.is_enabled ? "是" : "否",
           direction_str,
           status.confidence,
           status.frame_count);
}

// 内部函数实现

static int start_python_process(void)
{
    char command[512];
    
    if (debug_mode) {
        printf("[AI] 启动Python进程...\n");
    }
    
    // 检查Python3是否可用
    int ret = system("python3 --version > /dev/null 2>&1");
    if (ret != 0) {
        printf("[AI] 错误: Python3 不可用\n");
        return -1;
    }
    
    // 检查AI脚本文件是否存在
    if (access(AI_SCRIPT_PATH, F_OK) != 0) {
        printf("[AI] 错误: AI脚本文件不存在: %s\n", AI_SCRIPT_PATH);
        return -1;
    }
    
    // 构建启动命令 - 使用后台进程方式
    snprintf(command, sizeof(command), 
             "cd %s && python3 %s --daemon --status-file=/tmp/ai_status.json > /tmp/ai.log 2>&1 &",
             ".", AI_SCRIPT_PATH);
    
    if (debug_mode) {
        printf("[AI] 执行命令: %s\n", command);
    }
    
    // 启动Python进程
    ret = system(command);
    if (ret != 0) {
        printf("[AI] 错误: 无法启动Python进程 (返回码: %d)\n", ret);
        return -1;
    }
    
    // 等待进程启动
    sleep(2);
    
    // 查找Python进程PID
    FILE* fp = popen("pgrep -f 'python3.*ai.py'", "r");
    if (fp) {
        char pid_str[32];
        if (fgets(pid_str, sizeof(pid_str), fp)) {
            python_pid = atoi(pid_str);
            if (debug_mode) {
                printf("[AI] 找到Python进程PID: %d\n", python_pid);
            }
        }
        pclose(fp);
    }
    
    if (python_pid <= 0) {
        printf("[AI] 警告: 无法获取Python进程PID，但进程可能已启动\n");
        // 不返回错误，继续执行
        python_pid = 1; // 设置一个虚拟PID
    }
    
    if (debug_mode) {
        printf("[AI] Python进程启动成功\n");
    }
    
    return 0;
}

static int stop_python_process(void)
{
    if (debug_mode) {
        printf("[AI] 停止Python进程...\n");
    }
    
    if (python_pid > 1) { // 只有真实PID才尝试终止
        if (debug_mode) {
            printf("[AI] 终止Python进程 (PID: %d)\n", python_pid);
        }
        
        // 先尝试友好关闭
        kill(python_pid, SIGTERM);
        
        // 等待一段时间
        sleep(1);
        
        // 检查进程是否还存在
        if (kill(python_pid, 0) == 0) {
            // 强制终止
            kill(python_pid, SIGKILL);
            if (debug_mode) {
                printf("[AI] 强制终止Python进程\n");
            }
        }
        
        // 等待进程完全结束
        int status;
        waitpid(python_pid, &status, WNOHANG);
    } else {
        // 尝试通过进程名终止
        if (debug_mode) {
            printf("[AI] 通过进程名终止Python进程\n");
        }
        system("pkill -f 'python3.*ai.py' 2>/dev/null");
    }
    
    python_pid = -1;
    
    if (python_pipe) {
        pclose(python_pipe);
        python_pipe = NULL;
    }
    
    if (debug_mode) {
        printf("[AI] Python进程已停止\n");
    }
    
    return 0;
}

static int send_python_command(const char* command)
{
    if (!python_pipe || !command) {
        return -1;
    }
    
    fprintf(python_pipe, "%s\n", command);
    fflush(python_pipe);
    
    if (debug_mode) {
        printf("[AI] 发送命令: %s\n", command);
    }
    
    return 0;
}

static int read_python_status(void)
{
    // 简化实现：在实际项目中，可以通过共享内存、管道或其他IPC机制
    // 从Python进程读取状态信息
    return 0;
}

static void update_status(ai_state_t state, int enabled, ai_direction_t direction, float confidence)
{
    pthread_mutex_lock(&status_mutex);
    current_status.state = state;
    current_status.is_enabled = enabled;
    current_status.current_direction = direction;
    current_status.confidence = confidence;
    current_status.last_update = time(NULL);
    pthread_mutex_unlock(&status_mutex);
}
