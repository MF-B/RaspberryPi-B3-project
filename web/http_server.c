#include "http_server.h"
#include "../components/beep.h"
#include "../components/rgb.h"
#include "../components/servo.h"
#include "../components/temp.h"
#include "../components/distance.h"
#include "../components/clock.h"
#include <signal.h>
#include <cjson/cJSON.h>

// 全局变量
volatile int server_running = 1;
static int server_socket = -1;

// 信号处理函数
void signal_handler(int signal) {
    printf("\n接收到信号 %d，正在关闭服务器...\n", signal);
    server_running = 0;
    if (server_socket != -1) {
        close(server_socket);
    }
}

// 设置信号处理
void setup_signal_handlers(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

// 启动HTTP服务器
int start_server(int port) {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len;
    
    // 创建socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("创建socket失败");
        return -1;
    }
    
    server_socket = sockfd;
    
    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("设置socket选项失败");
        close(sockfd);
        return -1;
    }
    
    // 绑定地址
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("绑定地址失败");
        close(sockfd);
        return -1;
    }
    
    // 监听连接
    if (listen(sockfd, MAX_CONNECTIONS) < 0) {
        perror("监听失败");
        close(sockfd);
        return -1;
    }
    
    printf("HTTP服务器启动成功，监听端口: %d\n", port);
    printf("访问地址: http://localhost:%d\n", port);
    
    // 主循环
    while (server_running) {
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len);
        
        if (newsockfd < 0) {
            if (server_running) {
                perror("接受连接失败");
            }
            continue;
        }
        
        // 创建客户端连接结构
        client_connection_t *client = malloc(sizeof(client_connection_t));
        if (client) {
            client->socket_fd = newsockfd;
            client->address = cli_addr;
            
            // 创建线程处理客户端请求
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void *)client);
            pthread_detach(thread);
        } else {
            close(newsockfd);
        }
    }
    
    close(sockfd);
    return 0;
}

// 处理客户端请求
void *handle_client(void *arg) {
    client_connection_t *client = (client_connection_t *)arg;
    char buffer[BUFFER_SIZE];
    http_request_t request;
    http_response_t response;
    
    // 读取请求数据
    int bytes_read = recv(client->socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        close(client->socket_fd);
        free(client);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    
    // 解析HTTP请求
    if (parse_http_request(buffer, &request) != 0) {
        create_error_response(&response, 400, "Bad Request");
        send_http_response(client->socket_fd, &response);
        close(client->socket_fd);
        free(client);
        return NULL;
    }
    
    // 路由处理
    if (strncmp(request.path, "/api/", 5) == 0) {
        handle_api_request(&request, &response);
    } else {
        handle_static_file(&request, &response);
    }
    
    // 发送响应
    send_http_response(client->socket_fd, &response);
    
    close(client->socket_fd);
    free(client);
    return NULL;
}

// 解析HTTP请求
int parse_http_request(const char *request_data, http_request_t *request) {
    char *line, *saveptr;
    char *request_copy = strdup(request_data);
    int first_line = 1;
    
    memset(request, 0, sizeof(http_request_t));
    
    // 找到头部和身体的分界线（空行）
    char *body_start = strstr(request_data, "\r\n\r\n");
    if (body_start) {
        body_start += 4; // 跳过 \r\n\r\n
    }
    
    line = strtok_r(request_copy, "\r\n", &saveptr);
    while (line != NULL) {
        if (first_line) {
            // 解析请求行: METHOD PATH HTTP/1.1
            char *method = strtok(line, " ");
            char *path = strtok(NULL, " ");
            
            if (method && path) {
                strncpy(request->method, method, sizeof(request->method) - 1);
                
                // 分离路径和查询字符串
                char *query_start = strchr(path, '?');
                if (query_start) {
                    *query_start = '\0';
                    query_start++;
                    strncpy(request->query, query_start, sizeof(request->query) - 1);
                }
                strncpy(request->path, path, sizeof(request->path) - 1);
            }
            first_line = 0;
        } else if (strncmp(line, "Content-Length:", 15) == 0) {
            request->content_length = atoi(line + 16);
        }
        line = strtok_r(NULL, "\r\n", &saveptr);
    }
    
    // 提取请求体
    if (body_start && request->content_length > 0) {
        int body_len = strlen(body_start);
        if (body_len > 0 && (size_t)body_len < sizeof(request->body)) {
            strncpy(request->body, body_start, body_len);
            request->body[body_len] = '\0';
            printf("解析到请求体: %s\n", request->body);
        }
    }
    
    free(request_copy);
    return 0;
}

// 发送HTTP响应
void send_http_response(int client_fd, http_response_t *response) {
    char header[1024];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        response->status_code, response->status_text,
        response->content_type, response->body_length);
    
    send(client_fd, header, strlen(header), 0);
    if (response->body_length > 0) {
        send(client_fd, response->body, response->body_length, 0);
    }
}

// 处理API请求
void handle_api_request(http_request_t *request, http_response_t *response) {
    printf("API请求: %s %s\n", request->method, request->path);
    
    if (strcmp(request->path, "/api/status") == 0) {
        api_get_status(request, response);
    } else if (strcmp(request->path, "/api/sensors") == 0) {
        api_get_sensors(request, response);
    } else if (strcmp(request->path, "/api/rgb") == 0) {
        api_control_rgb(request, response);
    } else if (strcmp(request->path, "/api/beep") == 0) {
        api_control_beep(request, response);
    } else if (strcmp(request->path, "/api/servo") == 0) {
        api_control_servo(request, response);
    } else if (strcmp(request->path, "/api/distance") == 0) {
        api_get_distance(request, response);
    } else if (strcmp(request->path, "/api/motion") == 0) {
        api_control_motion(request, response);  // 新增运动控制路由
    } else if (strcmp(request->path, "/api/camera") == 0) {
        api_camera_control(request, response);  // 新增摄像头控制路由
    } else {
        create_error_response(response, 404, "API Not Found");
    }
}

// 处理静态文件请求
void handle_static_file(http_request_t *request, http_response_t *response) {
    char filepath[512];
    
    // 默认文件
    if (strcmp(request->path, "/") == 0) {
        strcpy(request->path, "/index.html");
    }
    
    snprintf(filepath, sizeof(filepath), "web/static%s", request->path);
    
    // 检查文件是否存在
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        create_error_response(response, 404, "File Not Found");
        return;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        create_error_response(response, 500, "File Error");
        fclose(file);
        return;
    }
    
    if ((size_t)file_size > MAX_FILE_SIZE) {
        create_error_response(response, 500, "File Too Large");
        fclose(file);
        return;
    }
    
    // 读取文件内容
    response->body_length = fread(response->body, 1, (size_t)file_size, file);
    fclose(file);
    
    response->status_code = 200;
    strcpy(response->status_text, HTTP_OK);
    strcpy(response->content_type, get_mime_type(filepath));
}

// 获取MIME类型
const char *get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return MIME_PLAIN;
    
    if (strcmp(ext, ".html") == 0) return MIME_HTML;
    if (strcmp(ext, ".css") == 0) return MIME_CSS;
    if (strcmp(ext, ".js") == 0) return MIME_JS;
    if (strcmp(ext, ".json") == 0) return MIME_JSON;
    
    return MIME_PLAIN;
}

// 创建JSON响应
void create_json_response(http_response_t *response, const char *json_data) {
    response->status_code = 200;
    strcpy(response->status_text, HTTP_OK);
    strcpy(response->content_type, MIME_JSON);
    response->body_length = strlen(json_data);
    strcpy(response->body, json_data);
}

// 创建错误响应
void create_error_response(http_response_t *response, int status_code, const char *message) {
    response->status_code = status_code;
    
    switch (status_code) {
        case 400:
            strcpy(response->status_text, HTTP_BAD_REQUEST);
            break;
        case 404:
            strcpy(response->status_text, HTTP_NOT_FOUND);
            break;
        case 500:
            strcpy(response->status_text, HTTP_INTERNAL_ERROR);
            break;
        default:
            strcpy(response->status_text, "Unknown Error");
    }
    
    strcpy(response->content_type, MIME_JSON);
    snprintf(response->body, sizeof(response->body),
        "{\"error\":{\"code\":%d,\"message\":\"%s\"}}", status_code, message);
    response->body_length = strlen(response->body);
}

// 停止服务器
void stop_server(void) {
    server_running = 0;
}
