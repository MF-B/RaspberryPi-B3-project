#include "http_server.h"
#include "../components/beep.h"
#include "../components/rgb.h"
#include "../components/temp.h"
#include "../components/distance.h"
#include "../components/clock.h"
#include "../components/control.h"  // 新增运动控制
#include "../components/camera.h"   // 新增摄像头
#include "log.h"  // log.c 日志库
#include <cjson/cJSON.h>

// API: 获取系统状态
void api_get_status(http_request_t *request, http_response_t *response) {
    (void)request; // 避免未使用参数警告
    
    cJSON *json = cJSON_CreateObject();
    cJSON *status = cJSON_CreateString("online");
    cJSON *timestamp = cJSON_CreateNumber(time(NULL));
    cJSON *components = cJSON_CreateObject();
    
    // 组件状态
    cJSON_AddStringToObject(components, "beep", "ready");
    cJSON_AddStringToObject(components, "rgb", "ready");
    cJSON_AddStringToObject(components, "temp", "ready");
    cJSON_AddStringToObject(components, "distance", "ready");
    cJSON_AddStringToObject(components, "clock", "ready");
    cJSON_AddStringToObject(components, "control", "ready");  // 新增运动控制
    cJSON_AddStringToObject(components, "camera", camera_is_available() ? "ready" : "unavailable");  // 新增摄像头
    
    cJSON_AddItemToObject(json, "status", status);
    cJSON_AddItemToObject(json, "timestamp", timestamp);
    cJSON_AddItemToObject(json, "components", components);
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// API: 获取传感器数据
void api_get_sensors(http_request_t *request, http_response_t *response) {
    (void)request; // 避免未使用参数警告
    
    log_debug("API请求: GET /api/sensors");
    
    cJSON *json = cJSON_CreateObject();
    cJSON *sensors = cJSON_CreateObject();
    
    // 温度传感器数据 - 使用更短的超时时间避免阻塞
    temp_data_t sensor_data;
    log_debug("开始读取温度传感器数据...");
    int sensor_result = temp_read_with_retry(&sensor_data, 1); // 只重试1次，减少阻塞时间
    log_debug("温度传感器读取结果: %d", sensor_result);
    
    cJSON *temp = cJSON_CreateObject();
    if (sensor_result == TEMP_SUCCESS) {
        log_debug("温度传感器读取成功: 温度=%.1f°C, 湿度=%.1f%%", 
               sensor_data.temperature, sensor_data.humidity);
        cJSON_AddNumberToObject(temp, "temperature", sensor_data.temperature);
        cJSON_AddNumberToObject(temp, "humidity", sensor_data.humidity);
        cJSON_AddStringToObject(temp, "status", "success");
    } else {
        log_warn("温度传感器读取失败，错误码: %d", sensor_result);
        cJSON_AddNumberToObject(temp, "temperature", 0);
        cJSON_AddNumberToObject(temp, "humidity", 0);
        cJSON_AddStringToObject(temp, "status", "error");
    }
    cJSON_AddItemToObject(sensors, "temp", temp);
    
    // 距离传感器数据
    int distance = distance_read();
    cJSON *distance_sensor = cJSON_CreateObject();
    if (distance > 0) {
        cJSON_AddNumberToObject(distance_sensor, "distance", distance);
        cJSON_AddStringToObject(distance_sensor, "status", "success");
    } else {
        cJSON_AddNumberToObject(distance_sensor, "distance", 0);
        cJSON_AddStringToObject(distance_sensor, "status", "error");
    }
    cJSON_AddItemToObject(sensors, "distance", distance_sensor);
    
    // 时间戳
    cJSON_AddNumberToObject(json, "timestamp", time(NULL));
    cJSON_AddItemToObject(json, "sensors", sensors);
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// API: 控制RGB LED
void api_control_rgb(http_request_t *request, http_response_t *response) {
    log_debug("API请求: %s %s", request->method, request->path);
    log_debug("请求体: %s", strlen(request->body) > 0 ? request->body : "(empty)");
    
    cJSON *json = cJSON_CreateObject();
    
    if (strcmp(request->method, "POST") == 0) {
        // 解析POST数据
        log_debug("解析JSON数据...");
        cJSON *post_json = cJSON_Parse(request->body);
        if (!post_json) {
            log_error("JSON解析失败");
            create_error_response(response, 400, "Invalid JSON");
            return;
        }
        
        cJSON *action = cJSON_GetObjectItem(post_json, "action");
        if (!action || !cJSON_IsString(action)) {
            log_error("缺少action参数");
            create_error_response(response, 400, "Missing action parameter");
            cJSON_Delete(post_json);
            return;
        }
        
        const char *action_str = cJSON_GetStringValue(action);
        log_info("执行RGB动作: %s", action_str);
        
        if (strcmp(action_str, "on") == 0) {
            cJSON *color = cJSON_GetObjectItem(post_json, "color");
            if (color && cJSON_IsString(color)) {
                const char *color_str = cJSON_GetStringValue(color);
                log_info("设置RGB颜色: %s", color_str);
                
                if (strcmp(color_str, "red") == 0) {
                    log_debug("调用rgb_set_color(1, 0, 0)");
                    rgb_set_color(1, 0, 0);
                } else if (strcmp(color_str, "green") == 0) {
                    log_debug("调用rgb_set_color(0, 1, 0)");
                    rgb_set_color(0, 1, 0);
                } else if (strcmp(color_str, "blue") == 0) {
                    log_debug("调用rgb_set_color(0, 0, 1)");
                    rgb_set_color(0, 0, 1);
                } else if (strcmp(color_str, "white") == 0) {
                    log_debug("调用rgb_set_color(1, 1, 1)");
                    rgb_set_color(1, 1, 1);
                } else {
                    log_warn("未知颜色，使用默认白色");
                    rgb_set_color(1, 1, 1); // 默认白色
                }
                log_info("RGB设置完成");
                cJSON_AddStringToObject(json, "status", "success");
                cJSON_AddStringToObject(json, "message", "RGB LED turned on");
                cJSON_AddStringToObject(json, "color", color_str);
            } else {
                log_error("缺少color参数");
                create_error_response(response, 400, "Missing color parameter");
                cJSON_Delete(post_json);
                return;
            }
        } else if (strcmp(action_str, "off") == 0) {
            log_info("关闭RGB LED");
            rgb_set_color(0, 0, 0);
            log_info("RGB关闭完成");
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "RGB LED turned off");
        } else {
            log_error("无效的动作: %s", action_str);
            create_error_response(response, 400, "Invalid action");
            cJSON_Delete(post_json);
            return;
        }
        
        cJSON_Delete(post_json);
    } else {
        create_error_response(response, 405, "Method Not Allowed");
        cJSON_Delete(json);
        return;
    }
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// API: 控制蜂鸣器
void api_control_beep(http_request_t *request, http_response_t *response) {
    cJSON *json = cJSON_CreateObject();
    
    if (strcmp(request->method, "POST") == 0) {
        cJSON *post_json = cJSON_Parse(request->body);
        if (!post_json) {
            create_error_response(response, 400, "Invalid JSON");
            return;
        }
        
        cJSON *action = cJSON_GetObjectItem(post_json, "action");
        if (!action || !cJSON_IsString(action)) {
            create_error_response(response, 400, "Missing action parameter");
            cJSON_Delete(post_json);
            return;
        }
        
        const char *action_str = cJSON_GetStringValue(action);
        
        if (strcmp(action_str, "beep") == 0) {
            cJSON *duration_json = cJSON_GetObjectItem(post_json, "duration");
            int duration = 1000; // 默认1秒
            
            if (duration_json && cJSON_IsNumber(duration_json)) {
                duration = (int)cJSON_GetNumberValue(duration_json);
            }
            
            beep_on();
            usleep(duration * 1000); // 转换为微秒
            beep_off();
            
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "Beep completed");
            cJSON_AddNumberToObject(json, "duration", duration);
        } else {
            create_error_response(response, 400, "Invalid action");
            cJSON_Delete(post_json);
            return;
        }
        
        cJSON_Delete(post_json);
    } else {
        create_error_response(response, 405, "Method Not Allowed");
        cJSON_Delete(json);
        return;
    }
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// API: 获取距离数据
void api_get_distance(http_request_t *request, http_response_t *response) {
    (void)request; // 避免未使用参数警告
    
    cJSON *json = cJSON_CreateObject();
    
    int distance = distance_read();
    
    if (distance > 0) {
        cJSON_AddStringToObject(json, "status", "success");
        cJSON_AddNumberToObject(json, "distance", distance);
        cJSON_AddStringToObject(json, "unit", "cm");
    } else {
        cJSON_AddStringToObject(json, "status", "error");
        cJSON_AddStringToObject(json, "message", "Failed to read distance");
        cJSON_AddNumberToObject(json, "distance", 0);
    }
    
    cJSON_AddNumberToObject(json, "timestamp", time(NULL));
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}

// API: 运动控制
void api_control_motion(http_request_t *request, http_response_t *response) {
    cJSON *json = cJSON_CreateObject();
    
    if (strcmp(request->method, "POST") == 0) {
        cJSON *post_json = cJSON_Parse(request->body);
        if (!post_json) {
            create_error_response(response, 400, "Invalid JSON");
            return;
        }
        
        cJSON *action = cJSON_GetObjectItem(post_json, "action");
        if (!action || !cJSON_IsString(action)) {
            create_error_response(response, 400, "Missing action parameter");
            cJSON_Delete(post_json);
            return;
        }
        
        const char *action_str = cJSON_GetStringValue(action);
        
        // 获取可选参数
        cJSON *speed_json = cJSON_GetObjectItem(post_json, "speed");
        cJSON *duration_json = cJSON_GetObjectItem(post_json, "duration");
        
        int speed = 60; // 默认速度
        int duration = 0; // 默认持续时间(0表示持续运动)
        
        if (speed_json && cJSON_IsNumber(speed_json)) {
            speed = (int)cJSON_GetNumberValue(speed_json);
            if (speed < 0) speed = 0;
            if (speed > 100) speed = 100;
        }
        
        if (duration_json && cJSON_IsNumber(duration_json)) {
            duration = (int)cJSON_GetNumberValue(duration_json);
            if (duration < 0) duration = 0;
        }
        
        // 执行运动控制
        if (strcmp(action_str, "forward") == 0) {
            control_move_forward(speed);
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "Moving forward");
            cJSON_AddStringToObject(json, "action", "forward");
            cJSON_AddNumberToObject(json, "speed", speed);
        } else if (strcmp(action_str, "backward") == 0) {
            control_move_backward(speed);
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "Moving backward");
            cJSON_AddStringToObject(json, "action", "backward");
            cJSON_AddNumberToObject(json, "speed", speed);
        } else if (strcmp(action_str, "left") == 0) {
            control_turn_left(speed, duration);
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "Turning left");
            cJSON_AddStringToObject(json, "action", "left");
            cJSON_AddNumberToObject(json, "speed", speed);
            if (duration > 0) {
                cJSON_AddNumberToObject(json, "duration", duration);
            }
        } else if (strcmp(action_str, "right") == 0) {
            control_turn_right(speed, duration);
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "Turning right");
            cJSON_AddStringToObject(json, "action", "right");
            cJSON_AddNumberToObject(json, "speed", speed);
            if (duration > 0) {
                cJSON_AddNumberToObject(json, "duration", duration);
            }
        } else if (strcmp(action_str, "stop") == 0) {
            control_stop();
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "Stopped");
            cJSON_AddStringToObject(json, "action", "stop");
        } else {
            create_error_response(response, 400, "Invalid action");
            cJSON_Delete(post_json);
            return;
        }
        
        cJSON_Delete(post_json);
    } else if (strcmp(request->method, "GET") == 0) {
        // 获取当前运动状态
        motion_state_t state = get_motion_state();
        
        cJSON_AddStringToObject(json, "status", "success");
        cJSON_AddNumberToObject(json, "left_speed", state.left_speed);
        cJSON_AddNumberToObject(json, "right_speed", state.right_speed);
        cJSON_AddNumberToObject(json, "is_moving", state.is_moving);
        
        const char *motion_name = "unknown";
        switch (state.current_motion) {
            case MOTION_STOP: motion_name = "stop"; break;
            case MOTION_FORWARD: motion_name = "forward"; break;
            case MOTION_BACKWARD: motion_name = "backward"; break;
            case MOTION_LEFT: motion_name = "left"; break;
            case MOTION_RIGHT: motion_name = "right"; break;
            case MOTION_ACCELERATE: motion_name = "accelerate"; break;
            case MOTION_DECELERATE: motion_name = "decelerate"; break;
        }
        cJSON_AddStringToObject(json, "current_motion", motion_name);
    } else {
        create_error_response(response, 405, "Method Not Allowed");
        cJSON_Delete(json);
        return;
    }
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}


// API: 摄像头控制
void api_camera_control(http_request_t *request, http_response_t *response) {
    log_debug("API请求: %s %s", request->method, request->path);
    log_debug("请求体: %s", strlen(request->body) > 0 ? request->body : "(empty)");
    
    cJSON *json = cJSON_CreateObject();
    
    if (strcmp(request->method, "POST") == 0) {
        // 解析POST数据
        log_debug("解析摄像头控制JSON数据...");
        cJSON *post_json = cJSON_Parse(request->body);
        if (!post_json) {
            log_error("JSON解析失败");
            create_error_response(response, 400, "Invalid JSON");
            return;
        }
        
        cJSON *action = cJSON_GetObjectItem(post_json, "action");
        if (!action || !cJSON_IsString(action)) {
            log_error("缺少action参数");
            create_error_response(response, 400, "Missing action parameter");
            cJSON_Delete(post_json);
            return;
        }
        
        const char *action_str = cJSON_GetStringValue(action);
        log_info("执行摄像头动作: %s", action_str);
        
        if (strcmp(action_str, "snapshot") == 0) {
            log_info("拍摄快照");
            if (camera_take_snapshot() == 0) {
                log_info("拍摄快照成功");
                cJSON_AddStringToObject(json, "status", "success");
                cJSON_AddStringToObject(json, "message", "Snapshot taken");
                cJSON_AddStringToObject(json, "image_url", "/images/snapshot.jpg");
            } else {
                log_error("拍摄快照失败");
                cJSON_AddStringToObject(json, "status", "error");
                cJSON_AddStringToObject(json, "message", "Failed to take snapshot");
            }
        } else {
            log_warn("无效的摄像头动作: %s", action_str);
            create_error_response(response, 400, "Invalid action");
            cJSON_Delete(post_json);
            return;
        }
        
        cJSON_Delete(post_json);
    } else if (strcmp(request->method, "GET") == 0) {
        // 获取摄像头状态
        camera_state_t state = camera_get_state();
        
        cJSON_AddStringToObject(json, "status", "success");
        
        cJSON *camera_info = cJSON_CreateObject();
        const char *status_str = "unknown";
        switch (state.status) {
            case CAMERA_STATUS_STOPPED: status_str = "stopped"; break;
            case CAMERA_STATUS_RUNNING: status_str = "running"; break;
            case CAMERA_STATUS_ERROR: status_str = "error"; break;
        }
        cJSON_AddStringToObject(camera_info, "status", status_str);
        cJSON_AddBoolToObject(camera_info, "available", camera_is_available());
        cJSON_AddNumberToObject(camera_info, "frame_count", state.frame_count);
        
        cJSON_AddItemToObject(json, "camera", camera_info);
    } else {
        create_error_response(response, 405, "Method Not Allowed");
        cJSON_Delete(json);
        return;
    }
    
    char *json_string = cJSON_Print(json);
    create_json_response(response, json_string);
    
    free(json_string);
    cJSON_Delete(json);
}
