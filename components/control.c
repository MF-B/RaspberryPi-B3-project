#include "control.h"

// PWM范围
#define PWM_RANGE 100

// 全局运动状态
static motion_state_t current_state = {0, 0, 0, MOTION_STOP};

// 内部辅助函数
static void wheel_set_motor(int pin_pos, int pin_neg, int speed, int direction);
static void update_motion_state(int left_speed, int right_speed, motion_type_t motion);

// 设置单个电机的速度和方向
static void wheel_set_motor(int pin_pos, int pin_neg, int speed, int direction)
{
    // 限制速度范围
    if (speed < 0) speed = 0;
    if (speed > WHEEL_MAX_SPEED) speed = WHEEL_MAX_SPEED;
    
    if (direction == 1) {
        // 正向
        softPwmWrite(pin_pos, speed);
        softPwmWrite(pin_neg, 0);
    } else if (direction == -1) {
        // 反向
        softPwmWrite(pin_pos, 0);
        softPwmWrite(pin_neg, speed);
    } else {
        // 停止
        softPwmWrite(pin_pos, 0);
        softPwmWrite(pin_neg, 0);
    }
}

// 初始化车轮控制模块
void wheel_init(void)
{
    // 设置引脚模式
    pinMode(WHEEL_LP, OUTPUT);
    pinMode(WHEEL_LN, OUTPUT);
    pinMode(WHEEL_RP, OUTPUT);
    pinMode(WHEEL_RN, OUTPUT);
    
    // 初始化软件PWM
    softPwmCreate(WHEEL_LP, 0, PWM_RANGE);
    softPwmCreate(WHEEL_LN, 0, PWM_RANGE);
    softPwmCreate(WHEEL_RP, 0, PWM_RANGE);
    softPwmCreate(WHEEL_RN, 0, PWM_RANGE);
    
    printf("车轮控制初始化完成 (引脚 LP=%d LN=%d RP=%d RN=%d)\n", 
           WHEEL_LP, WHEEL_LN, WHEEL_RP, WHEEL_RN);
}

// 启动车轮（默认前进）
void wheel_on(void)
{
    wheel_forward(WHEEL_DEFAULT_SPEED);
    printf("车轮控制: 启动\n");
}

// 停止车轮
void wheel_off(void)
{
    wheel_set_motor(WHEEL_LP, WHEEL_LN, 0, 0);
    wheel_set_motor(WHEEL_RP, WHEEL_RN, 0, 0);
    update_motion_state(0, 0, MOTION_STOP);
    printf("车轮控制: 停止\n");
}

// 清理车轮控制模块
void wheel_cleanup(void)
{
    wheel_off();
    printf("车轮控制清理完成\n");
}

// 前进
void wheel_forward(int speed)
{
    if (speed < 0) speed = 0;
    if (speed > WHEEL_MAX_SPEED) speed = WHEEL_MAX_SPEED;
    
    wheel_set_motor(WHEEL_LP, WHEEL_LN, speed, -1);
    wheel_set_motor(WHEEL_RP, WHEEL_RN, speed, -1);
    update_motion_state(speed, speed, MOTION_FORWARD);
    printf("车轮控制: 前进 - 速度 %d\n", speed);
}

// 后退
void wheel_backward(int speed)
{
    if (speed < 0) speed = 0;
    if (speed > WHEEL_MAX_SPEED) speed = WHEEL_MAX_SPEED;
    
    wheel_set_motor(WHEEL_LP, WHEEL_LN, speed, 1);
    wheel_set_motor(WHEEL_RP, WHEEL_RN, speed, 1);
    update_motion_state(speed, speed, MOTION_BACKWARD);
    printf("车轮控制: 后退 - 速度 %d\n", speed);
}

// 左转
void wheel_left(int speed)
{
    if (speed < 0) speed = 0;
    if (speed > WHEEL_MAX_SPEED) speed = WHEEL_MAX_SPEED;
    
    wheel_set_motor(WHEEL_LP, WHEEL_LN, speed, 1);
    wheel_set_motor(WHEEL_RP, WHEEL_RN, speed, -1);
    update_motion_state(speed, speed, MOTION_LEFT);
    printf("车轮控制: 左转 - 速度 %d\n", speed);
}

// 右转
void wheel_right(int speed)
{
    if (speed < 0) speed = 0;
    if (speed > WHEEL_MAX_SPEED) speed = WHEEL_MAX_SPEED;
    
    wheel_set_motor(WHEEL_LP, WHEEL_LN, speed, -1);
    wheel_set_motor(WHEEL_RP, WHEEL_RN, speed, 1);
    update_motion_state(speed, speed, MOTION_RIGHT);
    printf("车轮控制: 右转 - 速度 %d\n", speed);
}

// 更新运动状态的内部函数
static void update_motion_state(int left_speed, int right_speed, motion_type_t motion)
{
    current_state.left_speed = left_speed;
    current_state.right_speed = right_speed;
    current_state.is_moving = (left_speed != 0 || right_speed != 0) ? 1 : 0;
    current_state.current_motion = motion;
}

// 获取当前运动状态
motion_state_t get_motion_state(void)
{
    return current_state;
}
