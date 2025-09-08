#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

// 服务器配置
#define SERVER_PORT 8080
#define BUFFER_SIZE 8192
#define MAX_CONNECTIONS 10
#define MAX_FILE_SIZE 65536  // 64KB - 最大文件大小

// HTTP状态码
#define HTTP_OK "200 OK"
#define HTTP_NOT_FOUND "404 Not Found"
#define HTTP_BAD_REQUEST "400 Bad Request"
#define HTTP_INTERNAL_ERROR "500 Internal Server Error"

// MIME类型
#define MIME_HTML "text/html"
#define MIME_CSS "text/css"
#define MIME_JS "application/javascript"
#define MIME_JSON "application/json"
#define MIME_PLAIN "text/plain"

// HTTP请求结构
typedef struct {
    char method[16];
    char path[256];
    char query[512];
    char body[4096];  // 增加请求体大小
    int content_length;
} http_request_t;

// HTTP响应结构
typedef struct {
    int status_code;
    char status_text[64];
    char content_type[64];
    char body[MAX_FILE_SIZE];  // 使用更大的缓冲区
    int body_length;
} http_response_t;

// 客户端连接结构
typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} client_connection_t;

// 函数声明
int start_server(int port);
void *handle_client(void *arg);
int parse_http_request(const char *request_data, http_request_t *request);
void send_http_response(int client_fd, http_response_t *response);
void handle_api_request(http_request_t *request, http_response_t *response);
void handle_static_file(http_request_t *request, http_response_t *response);
const char *get_mime_type(const char *filename);
void url_decode(char *dst, const char *src);
void create_json_response(http_response_t *response, const char *json_data);
void create_error_response(http_response_t *response, int status_code, const char *message);

// API路由处理函数
void api_get_status(http_request_t *request, http_response_t *response);
void api_get_sensors(http_request_t *request, http_response_t *response);
void api_control_rgb(http_request_t *request, http_response_t *response);
void api_control_beep(http_request_t *request, http_response_t *response);
void api_control_servo(http_request_t *request, http_response_t *response);
void api_get_distance(http_request_t *request, http_response_t *response);
void api_control_motion(http_request_t *request, http_response_t *response);  // 新增运动控制
void api_camera_control(http_request_t *request, http_response_t *response);  // 新增摄像头控制

// 服务器状态
extern volatile int server_running;
void stop_server(void);
void setup_signal_handlers(void);

#endif // HTTP_SERVER_H
