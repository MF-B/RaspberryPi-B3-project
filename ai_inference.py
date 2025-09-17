#!/usr/bin/env python3
"""
AI推理模块 - 线路跟踪
用于TensorFlow Lite模型推理，识别前进、左转、右转等动作
"""

import os
import sys
import numpy as np
import cv2

# 尝试导入TensorFlow Lite
try:
    import tflite_runtime.interpreter as tflite
except ImportError:
    try:
        import tensorflow.lite as tflite
    except ImportError:
        print("错误: 无法导入TensorFlow Lite。请安装 tflite-runtime 或 tensorflow")
        sys.exit(1)

class LineFollowerAI:
    """线路跟踪AI推理类"""
    
    def __init__(self, model_path="model/line_follower.tflite"):
        """
        初始化AI推理器
        
        Args:
            model_path (str): TFLite模型文件路径
        """
        self.model_path = model_path
        self.interpreter = None
        self.input_details = None
        self.output_details = None
        self.input_shape = None
        self.is_initialized = False
        
        # 类别定义 (根据您的模型调整)
        self.class_names = {
            0: "left",      # 左转
            1: "forward",   # 直行
            2: "right",     # 右转
            3: "stop"       # 停止 (可选)
        }
        
    def initialize(self):
        """初始化TensorFlow Lite解释器"""
        try:
            if not os.path.exists(self.model_path):
                print(f"错误: 模型文件不存在: {self.model_path}")
                return False
                
            # 加载模型
            self.interpreter = tflite.Interpreter(model_path=self.model_path)
            self.interpreter.allocate_tensors()
            
            # 获取输入输出详情
            self.input_details = self.interpreter.get_input_details()
            self.output_details = self.interpreter.get_output_details()
            
            # 获取输入形状 (通常是 [1, height, width, channels])
            self.input_shape = self.input_details[0]['shape']
            
            print(f"AI模型加载成功:")
            print(f"  模型路径: {self.model_path}")
            print(f"  输入形状: {self.input_shape}")
            print(f"  输出形状: {self.output_details[0]['shape']}")
            
            self.is_initialized = True
            return True
            
        except Exception as e:
            print(f"AI模型初始化失败: {e}")
            return False
    
    def preprocess_image(self, image):
        """
        预处理输入图像
        
        Args:
            image: OpenCV图像 (BGR格式)
            
        Returns:
            np.ndarray: 预处理后的图像数据
        """
        if image is None or image.size == 0:
            return None
            
        try:
            # 获取目标尺寸 (去掉batch维度)
            target_height = self.input_shape[1]
            target_width = self.input_shape[2]
            
            # 调整图像大小
            resized = cv2.resize(image, (target_width, target_height))
            
            # 转换为RGB (OpenCV使用BGR)
            rgb_image = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
            
            # 归一化到 [0, 1]
            normalized = rgb_image.astype(np.float32) / 255.0
            
            # 添加batch维度
            input_data = np.expand_dims(normalized, axis=0)
            
            return input_data
            
        except Exception as e:
            print(f"图像预处理失败: {e}")
            return None
    
    def predict(self, image, confidence_threshold=0.7):
        """
        对图像进行推理预测
        
        Args:
            image: OpenCV图像 (BGR格式)
            confidence_threshold (float): 置信度阈值
            
        Returns:
            dict: 包含预测类别、置信度等信息的字典
        """
        if not self.is_initialized:
            return {"success": False, "error": "AI模型未初始化"}
            
        try:
            # 预处理图像
            input_data = self.preprocess_image(image)
            if input_data is None:
                return {"success": False, "error": "图像预处理失败"}
            
            # 设置输入数据
            self.interpreter.set_tensor(self.input_details[0]['index'], input_data)
            
            # 执行推理
            self.interpreter.invoke()
            
            # 获取输出
            output = self.interpreter.get_tensor(self.output_details[0]['index'])
            
            # 获取预测类别和置信度
            pred_class = np.argmax(output[0])
            confidence = np.max(output[0])
            
            # 获取类别名称
            class_name = self.class_names.get(pred_class, "unknown")
            
            result = {
                "success": True,
                "class_id": int(pred_class),
                "class_name": class_name,
                "confidence": float(confidence),
                "meets_threshold": confidence >= confidence_threshold,
                "all_scores": output[0].tolist()
            }
            
            return result
            
        except Exception as e:
            return {"success": False, "error": f"推理失败: {e}"}
    
    def predict_from_buffer(self, image_buffer, width, height, confidence_threshold=0.7):
        """
        从图像缓冲区进行推理预测 (供C++调用)
        
        Args:
            image_buffer: 图像数据缓冲区 (BGR格式)
            width (int): 图像宽度
            height (int): 图像高度
            confidence_threshold (float): 置信度阈值
            
        Returns:
            dict: 预测结果
        """
        try:
            # 从缓冲区创建OpenCV图像
            image_array = np.frombuffer(image_buffer, dtype=np.uint8)
            image = image_array.reshape((height, width, 3))
            
            return self.predict(image, confidence_threshold)
            
        except Exception as e:
            return {"success": False, "error": f"缓冲区解析失败: {e}"}
    
    def cleanup(self):
        """清理资源"""
        self.interpreter = None
        self.input_details = None
        self.output_details = None
        self.is_initialized = False
        print("AI推理器已清理")


# 全局AI实例 (供C接口使用)
_global_ai = LineFollowerAI()

def ai_init(model_path="model/line_follower.tflite"):
    """
    初始化AI推理器 (C接口)
    
    Args:
        model_path (str): 模型文件路径
        
    Returns:
        bool: 初始化是否成功
    """
    global _global_ai
    _global_ai = LineFollowerAI(model_path)
    return _global_ai.initialize()

def ai_predict_from_image(image_buffer, width, height, confidence_threshold=0.7):
    """
    从图像缓冲区进行预测 (C接口)
    
    Args:
        image_buffer: 图像数据
        width (int): 宽度
        height (int): 高度  
        confidence_threshold (float): 置信度阈值
        
    Returns:
        dict: 预测结果
    """
    global _global_ai
    return _global_ai.predict_from_buffer(image_buffer, width, height, confidence_threshold)

def ai_cleanup():
    """清理AI资源 (C接口)"""
    global _global_ai
    _global_ai.cleanup()

# 测试函数
def test_ai_inference():
    """测试AI推理功能"""
    ai = LineFollowerAI()
    
    if not ai.initialize():
        print("AI初始化失败")
        return False
    
    # 创建测试图像
    test_image = np.zeros((480, 640, 3), dtype=np.uint8)
    cv2.line(test_image, (320, 0), (320, 480), (255, 255, 255), 10)
    
    result = ai.predict(test_image)
    print(f"测试结果: {result}")
    
    ai.cleanup()
    return result["success"]

if __name__ == "__main__":
    # 直接运行时进行测试
    print("测试AI推理模块...")
    success = test_ai_inference()
    print(f"测试结果: {'成功' if success else '失败'}")
