/*
 * AI推理C封装模块头文件
 * 定义AI推理相关的数据结构和函数接口
 */

#ifndef AI_WRAPPER_H
#define AI_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// AI推理结果结构
typedef struct {
    int success;           // 是否成功 (1=成功, 0=失败)
    int class_id;          // 预测类别ID (0=left, 1=forward, 2=right, 3=stop)
    char class_name[32];   // 类别名称字符串
    float confidence;      // 置信度 (0.0-1.0)
    int meets_threshold;   // 是否满足置信度阈值 (1=满足, 0=不满足)
    char error_msg[256];   // 错误信息 (仅在success=0时有效)
} ai_result_t;

// AI类别定义
#define AI_CLASS_LEFT     0   // 左转
#define AI_CLASS_FORWARD  1   // 直行
#define AI_CLASS_RIGHT    2   // 右转
#define AI_CLASS_STOP     3   // 停止

// 默认置信度阈值
#define AI_DEFAULT_CONFIDENCE_THRESHOLD  0.7f

/**
 * 初始化AI封装模块
 * 
 * @param model_path TFLite模型文件路径，如果为NULL则使用默认路径
 * @return 1=成功, 0=失败
 */
int ai_wrapper_init(const char* model_path);

/**
 * 对图像数据进行AI推理
 * 
 * @param image_data 图像数据缓冲区 (BGR格式，3通道)
 * @param width 图像宽度
 * @param height 图像高度
 * @param confidence_threshold 置信度阈值 (0.0-1.0)
 * @param result 输出的推理结果
 * @return 1=成功, 0=失败
 */
int ai_wrapper_predict(const unsigned char* image_data, int width, int height, 
                      float confidence_threshold, ai_result_t* result);

/**
 * 清理AI封装模块，释放资源
 */
void ai_wrapper_cleanup(void);

/**
 * 根据类别ID获取动作名称
 * 
 * @param class_id 类别ID
 * @return 动作名称字符串
 */
const char* ai_get_action_name(int class_id);

/**
 * 打印AI推理结果 (调试用)
 * 
 * @param result AI推理结果指针
 */
void ai_print_result(const ai_result_t* result);

#ifdef __cplusplus
}
#endif

#endif // AI_WRAPPER_H
