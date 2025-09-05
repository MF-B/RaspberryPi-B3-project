#include "http_server.h"
#include "../components/beep.h"
#include "../components/rgb.h"
#include "../components/servo.h"
#include "../components/DHT.h"
#include "../components/usonic.h"
#include "../components/clock.h"
#include "../components/control.h"  // 新增运动控制
#include <cjson/cJSON.h>

// API: 获取系统状态
void api_get_status(http_request_t *request, http_response_t *response) {
    (void)request; // 避免未使用参数警告
    
    cJSON *json = cJSON_CreateObject();
    cJSON *status = cJSON_CreateString("online");
    cJSON *timestamp = cJSON_CreateNumber(time(NULL));
    cJSON *components = cJSON_CreateObject();
    
    // 组件状态
    cJSON_AddStringToObject(components, "rgb", "ready");
    cJSON_AddStringToObject(components, "beep", "ready");
    cJSON_AddStringToObject(components, "servo", "ready");
    cJSON_AddStringToObject(components, "dht11", "ready");
    cJSON_AddStringToObject(components, "ultrasonic", "ready");
    cJSON_AddStringToObject(components, "clock", "ready");
    cJSON_AddStringToObject(components, "motion", "ready");  // 新增运动控制
    
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
    
    cJSON *json = cJSON_CreateObject();
    cJSON *sensors = cJSON_CreateObject();
    
    // DHT11温湿度数据
    DHT11_Data dht_data;
    int dht_result = dht11_read_with_retry(&dht_data, 3);
    
    cJSON *dht = cJSON_CreateObject();
    if (dht_result == DHT_SUCCESS) {
        cJSON_AddNumberToObject(dht, "temperature", dht_data.temperature);
        cJSON_AddNumberToObject(dht, "humidity", dht_data.humidity);
        cJSON_AddStringToObject(dht, "status", "success");
    } else {
        cJSON_AddNumberToObject(dht, "temperature", 0);
        cJSON_AddNumberToObject(dht, "humidity", 0);
        cJSON_AddStringToObject(dht, "status", "error");
    }
    cJSON_AddItemToObject(sensors, "dht11", dht);
    
    // 超声波距离数据
    int distance = read_dist();
    cJSON *ultrasonic = cJSON_CreateObject();
    if (distance > 0) {
        cJSON_AddNumberToObject(ultrasonic, "distance", distance);
        cJSON_AddStringToObject(ultrasonic, "status", "success");
    } else {
        cJSON_AddNumberToObject(ultrasonic, "distance", 0);
        cJSON_AddStringToObject(ultrasonic, "status", "error");
    }
    cJSON_AddItemToObject(sensors, "ultrasonic", ultrasonic);
    
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
    cJSON *json = cJSON_CreateObject();
    
    if (strcmp(request->method, "POST") == 0) {
        // 解析POST数据
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
        
        if (strcmp(action_str, "on") == 0) {
            cJSON *color = cJSON_GetObjectItem(post_json, "color");
            if (color && cJSON_IsString(color)) {
                const char *color_str = cJSON_GetStringValue(color);
                if (strcmp(color_str, "red") == 0) {
                    rgb_set_color(1, 0, 0);
                } else if (strcmp(color_str, "green") == 0) {
                    rgb_set_color(0, 1, 0);
                } else if (strcmp(color_str, "blue") == 0) {
                    rgb_set_color(0, 0, 1);
                } else if (strcmp(color_str, "white") == 0) {
                    rgb_set_color(1, 1, 1);
                } else {
                    rgb_set_color(1, 1, 1); // 默认白色
                }
                cJSON_AddStringToObject(json, "status", "success");
                cJSON_AddStringToObject(json, "message", "RGB LED turned on");
                cJSON_AddStringToObject(json, "color", color_str);
            } else {
                create_error_response(response, 400, "Missing color parameter");
                cJSON_Delete(post_json);
                return;
            }
        } else if (strcmp(action_str, "off") == 0) {
            rgb_set_color(0, 0, 0);
            cJSON_AddStringToObject(json, "status", "success");
            cJSON_AddStringToObject(json, "message", "RGB LED turned off");
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

// API: 控制舵机
void api_control_servo(http_request_t *request, http_response_t *response) {
    cJSON *json = cJSON_CreateObject();
    
    if (strcmp(request->method, "POST") == 0) {
        cJSON *post_json = cJSON_Parse(request->body);
        if (!post_json) {
            create_error_response(response, 400, "Invalid JSON");
            return;
        }
        
        cJSON *angle_json = cJSON_GetObjectItem(post_json, "angle");
        if (!angle_json || !cJSON_IsNumber(angle_json)) {
            create_error_response(response, 400, "Missing angle parameter");
            cJSON_Delete(post_json);
            return;
        }
        
        int angle = (int)cJSON_GetNumberValue(angle_json);
        
        if (angle < 0 || angle > 180) {
            create_error_response(response, 400, "Angle must be between 0 and 180");
            cJSON_Delete(post_json);
            return;
        }
        
        servo_set_angle(angle);
        
        cJSON_AddStringToObject(json, "status", "success");
        cJSON_AddStringToObject(json, "message", "Servo angle set");
        cJSON_AddNumberToObject(json, "angle", angle);
        
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
    
    int distance = read_dist();
    
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
