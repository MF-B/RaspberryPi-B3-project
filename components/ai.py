#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AI 黑线寻迹模块
使用 TensorFlow Lite 模型进行推理
兼容树莓派B3硬件控制系统
"""

import tflite_runtime.interpreter as tflite
import cv2
import numpy as np
import threading
import time
import sys
import os
import logging
import ctypes
from ctypes import c_int, c_float, c_char_p, POINTER

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class AILineFollower:
    """AI黑线寻迹类"""
    
    def __init__(self, model_path="./model/line_follower.tflite", camera_index=0):
        """
        初始化AI寻迹模块
        
        Args:
            model_path: TFLite模型文件路径
            camera_index: 摄像头索引
        """
        self.model_path = model_path
        self.camera_index = camera_index
        self.interpreter = None
        self.cap = None
        self.input_details = None
        self.output_details = None
        
        # 控制状态
        self.is_running = False
        self.is_ai_enabled = False
        self.current_direction = "stop"
        self.confidence = 0.0
        self.frame_count = 0
        
        # 线程安全
        self.lock = threading.Lock()
        self.ai_thread = None
        
        # 预测类别映射
        self.class_mapping = {
            0: "left",      # 左转
            1: "forward",   # 直行(中间)
            2: "right",     # 右转
            3: "stop"       # 停止/未检测到线
        }
        
        # 加载控制库(可选，如果需要直接控制硬件)
        self.control_lib = None
        self._load_control_lib()
        
        logger.info("AI寻迹模块初始化完成")
    
    def _load_control_lib(self):
        """加载控制库"""
        try:
            # 尝试加载控制动态库
            lib_path = "./lib/control.so"
            if os.path.exists(lib_path):
                self.control_lib = ctypes.CDLL(lib_path)
                # 定义函数签名
                self.control_lib.wheel_forward.argtypes = [c_int]
                self.control_lib.wheel_left.argtypes = [c_int] 
                self.control_lib.wheel_right.argtypes = [c_int]
                self.control_lib.wheel_off.argtypes = []
                logger.info("控制库加载成功")
            else:
                logger.warning(f"控制库文件不存在: {lib_path}")
        except Exception as e:
            logger.warning(f"加载控制库失败: {e}")
    
    def initialize_model(self):
        """初始化TFLite模型"""
        try:
            if not os.path.exists(self.model_path):
                raise FileNotFoundError(f"模型文件不存在: {self.model_path}")
            
            # 加载模型
            self.interpreter = tflite.Interpreter(model_path=self.model_path)
            self.interpreter.allocate_tensors()
            
            # 获取输入输出信息
            self.input_details = self.interpreter.get_input_details()
            self.output_details = self.interpreter.get_output_details()
            
            logger.info(f"模型加载成功: {self.model_path}")
            logger.info(f"输入形状: {self.input_details[0]['shape']}")
            logger.info(f"输出形状: {self.output_details[0]['shape']}")
            
            return True
        except Exception as e:
            logger.error(f"模型初始化失败: {e}")
            return False
    
    def initialize_camera(self):
        """初始化摄像头"""
        try:
            self.cap = cv2.VideoCapture(self.camera_index)
            if not self.cap.isOpened():
                raise RuntimeError(f"无法打开摄像头 {self.camera_index}")
            
            # 设置摄像头参数
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.cap.set(cv2.CAP_PROP_FPS, 30)
            
            logger.info(f"摄像头初始化成功: {self.camera_index}")
            return True
        except Exception as e:
            logger.error(f"摄像头初始化失败: {e}")
            return False
    
    def preprocess_frame(self, frame):
        """预处理图像帧"""
        try:
            # 获取模型期望的输入尺寸
            input_shape = self.input_details[0]['shape']
            height, width = input_shape[1], input_shape[2]
            
            # 调整大小
            processed_frame = cv2.resize(frame, (width, height))
            
            # 归一化
            processed_frame = processed_frame.astype(np.float32) / 255.0
            
            # 添加批次维度
            processed_frame = np.expand_dims(processed_frame, axis=0)
            
            return processed_frame
        except Exception as e:
            logger.error(f"图像预处理失败: {e}")
            return None
    
    def predict(self, frame):
        """进行AI推理"""
        try:
            # 预处理
            input_frame = self.preprocess_frame(frame)
            if input_frame is None:
                return None, 0.0
            
            # 推理
            self.interpreter.set_tensor(self.input_details[0]['index'], input_frame)
            self.interpreter.invoke()
            
            # 获取输出
            output = self.interpreter.get_tensor(self.output_details[0]['index'])
            
            # 获取预测类别和置信度
            pred_class = np.argmax(output)
            confidence = np.max(output)
            
            direction = self.class_mapping.get(pred_class, "stop")
            
            return direction, float(confidence)
        except Exception as e:
            logger.error(f"AI推理失败: {e}")
            return "stop", 0.0
    
    def control_robot(self, direction, confidence, speed=50):
        """控制机器人运动"""
        if not self.control_lib:
            logger.debug(f"预测方向: {direction}, 置信度: {confidence:.3f}")
            return
        
        try:
            # 只有置信度足够高才执行控制
            if confidence > 0.7:
                if direction == "forward":
                    self.control_lib.wheel_forward(speed)
                elif direction == "left":
                    self.control_lib.wheel_left(speed)
                elif direction == "right":
                    self.control_lib.wheel_right(speed)
                else:
                    self.control_lib.wheel_off()
            else:
                # 置信度不够，停止
                self.control_lib.wheel_off()
                
            logger.debug(f"执行动作: {direction}, 置信度: {confidence:.3f}")
        except Exception as e:
            logger.error(f"机器人控制失败: {e}")
    
    def ai_thread_function(self):
        """AI推理线程主函数"""
        logger.info("AI推理线程启动")
        
        while self.is_running:
            try:
                if not self.is_ai_enabled:
                    time.sleep(0.1)
                    continue
                
                # 读取摄像头帧
                ret, frame = self.cap.read()
                if not ret:
                    logger.warning("无法读取摄像头帧")
                    time.sleep(0.1)
                    continue
                
                # AI推理
                direction, confidence = self.predict(frame)
                
                # 更新状态
                with self.lock:
                    self.current_direction = direction
                    self.confidence = confidence
                    self.frame_count += 1
                
                # 控制机器人(如果启用了直接控制)
                if direction and confidence:
                    self.control_robot(direction, confidence)
                
                # 控制帧率
                time.sleep(0.033)  # 约30FPS
                
            except Exception as e:
                logger.error(f"AI线程运行错误: {e}")
                time.sleep(1)
        
        logger.info("AI推理线程结束")
    
    def start(self):
        """启动AI模块"""
        if self.is_running:
            logger.warning("AI模块已经在运行")
            return False
        
        # 初始化模型和摄像头
        if not self.initialize_model():
            return False
        
        if not self.initialize_camera():
            return False
        
        # 启动线程
        self.is_running = True
        self.ai_thread = threading.Thread(target=self.ai_thread_function, daemon=True)
        self.ai_thread.start()
        
        logger.info("AI模块启动成功")
        return True
    
    def stop(self):
        """停止AI模块"""
        logger.info("正在停止AI模块...")
        
        # 停止AI推理
        self.is_ai_enabled = False
        self.is_running = False
        
        # 等待线程结束
        if self.ai_thread and self.ai_thread.is_alive():
            self.ai_thread.join(timeout=2)
        
        # 清理资源
        if self.cap:
            self.cap.release()
        
        # 停止机器人
        if self.control_lib:
            try:
                self.control_lib.wheel_off()
            except:
                pass
        
        logger.info("AI模块已停止")
    
    def enable_ai(self):
        """启用AI寻迹"""
        with self.lock:
            self.is_ai_enabled = True
        logger.info("AI寻迹已启用")
    
    def disable_ai(self):
        """禁用AI寻迹"""
        with self.lock:
            self.is_ai_enabled = False
        
        # 停止机器人
        if self.control_lib:
            try:
                self.control_lib.wheel_off()
            except:
                pass
        
        logger.info("AI寻迹已禁用")
    
    def get_status(self):
        """获取当前状态"""
        with self.lock:
            return {
                "is_running": self.is_running,
                "is_ai_enabled": self.is_ai_enabled,
                "current_direction": self.current_direction,
                "confidence": self.confidence,
                "frame_count": self.frame_count
            }

# 全局AI实例
ai_follower = None

def init_ai():
    """初始化AI模块"""
    global ai_follower
    try:
        ai_follower = AILineFollower()
        return ai_follower.start()
    except Exception as e:
        logger.error(f"AI模块初始化失败: {e}")
        return False

def cleanup_ai():
    """清理AI模块"""
    global ai_follower
    if ai_follower:
        ai_follower.stop()
        ai_follower = None

def enable_ai_following():
    """启用AI寻迹"""
    global ai_follower
    if ai_follower:
        ai_follower.enable_ai()
        return True
    return False

def disable_ai_following():
    """禁用AI寻迹"""
    global ai_follower
    if ai_follower:
        ai_follower.disable_ai()
        return True
    return False

def get_ai_status():
    """获取AI状态"""
    global ai_follower
    if ai_follower:
        return ai_follower.get_status()
    return {
        "is_running": False,
        "is_ai_enabled": False,
        "current_direction": "stop",
        "confidence": 0.0,
        "frame_count": 0
    }

# 如果直接运行此脚本，进行测试
if __name__ == "__main__":
    import argparse
    import json
    import signal
    
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='AI黑线寻迹模块')
    parser.add_argument('--daemon', action='store_true', help='以守护进程模式运行')
    parser.add_argument('--status-file', default='/tmp/ai_status.json', help='状态文件路径')
    parser.add_argument('--model', default='./model/line_follower.tflite', help='模型文件路径')
    parser.add_argument('--camera', type=int, default=0, help='摄像头索引')
    
    args = parser.parse_args()
    
    # 全局变量
    ai = None
    status_file = args.status_file
    
    def signal_handler(signum, frame):
        """信号处理器"""
        global ai
        if signum == signal.SIGUSR1:
            # 启用AI寻迹
            if ai:
                ai.enable_ai()
                logger.info("收到启用信号")
        elif signum == signal.SIGUSR2:
            # 禁用AI寻迹
            if ai:
                ai.disable_ai()
                logger.info("收到禁用信号")
        elif signum in [signal.SIGTERM, signal.SIGINT]:
            # 退出信号
            logger.info("收到退出信号")
            if ai:
                ai.stop()
            sys.exit(0)
    
    def write_status_file():
        """写入状态文件"""
        global ai, status_file
        if ai:
            try:
                status = ai.get_status()
                with open(status_file, 'w') as f:
                    json.dump(status, f)
            except Exception as e:
                logger.error(f"写入状态文件失败: {e}")
    
    # 设置信号处理器
    signal.signal(signal.SIGUSR1, signal_handler)
    signal.signal(signal.SIGUSR2, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    try:
        # 创建AI实例
        ai = AILineFollower(model_path=args.model, camera_index=args.camera)
        
        if args.daemon:
            # 守护进程模式
            logger.info("启动守护进程模式")
            
            if ai.start():
                logger.info("AI模块启动成功")
                
                # 主循环
                while True:
                    write_status_file()
                    time.sleep(1)
            else:
                logger.error("AI模块启动失败")
                sys.exit(1)
        else:
            # 交互模式 - 从stdin读取命令
            if ai.start():
                logger.info("AI模块启动成功，等待命令...")
                
                # 创建命令处理线程
                def command_handler():
                    while ai.is_running:
                        try:
                            line = sys.stdin.readline()
                            if not line:
                                break
                            command = line.strip().lower()
                            
                            if command == "enable":
                                ai.enable_ai()
                                logger.info("AI寻迹已启用")
                            elif command == "disable":
                                ai.disable_ai()
                                logger.info("AI寻迹已禁用")
                            elif command == "status":
                                status = ai.get_status()
                                logger.info(f"状态: {status}")
                            elif command == "quit" or command == "exit":
                                break
                        except Exception as e:
                            logger.error(f"命令处理错误: {e}")
                
                # 启动命令处理线程
                cmd_thread = threading.Thread(target=command_handler, daemon=True)
                cmd_thread.start()
                
                # 主循环
                while ai.is_running:
                    time.sleep(0.1)
            else:
                logger.error("AI模块启动失败")
                sys.exit(1)
    
    except KeyboardInterrupt:
        logger.info("用户中断")
    except Exception as e:
        logger.error(f"程序运行错误: {e}")
    finally:
        if ai:
            ai.stop()
        try:
            cv2.destroyAllWindows()
        except:
            pass
