#ifndef SENSORS_H
#define SENSORS_H

// ====================
// 传感器配置定义
// ====================

// 温度传感器返回值定义
#define TEMP_SUCCESS         1   // 成功读取数据
#define TEMP_CHECKSUM_ERROR  0   // 校验和错误
#define TEMP_NO_RESPONSE     2   // 传感器无响应
#define TEMP_TIMEOUT_ERROR   3   // 数据读取超时

// 温度传感器数据结构
typedef struct {
    float humidity;     // 湿度
    float temperature;  // 温度
    unsigned char raw_data[5]; // 原始数据
} temp_data_t;

// 摄像头状态枚举
typedef enum {
    CAMERA_STATUS_STOPPED = 0,
    CAMERA_STATUS_RUNNING,
    CAMERA_STATUS_ERROR
} camera_status_t;

// 摄像头状态结构
typedef struct {
    camera_status_t status;
    int stream_running;
    int frame_count;
} camera_state_t;

#endif
