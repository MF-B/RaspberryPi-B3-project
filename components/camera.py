#!/usr/bin/env python3
import cv2
import sys
import os
import time
import threading
from datetime import datetime
import json

class CameraController:
    def __init__(self):
        self.camera = None
        self.is_streaming = False
        self.stream_thread = None
        self.output_dir = "web/static/images"
        self.snapshot_path = os.path.join(self.output_dir, "snapshot.jpg")
        self.stream_path = os.path.join(self.output_dir, "stream.jpg")
        self.frame_count = 0
        self.last_frame_time = None
        
        # 确保输出目录存在
        os.makedirs(self.output_dir, exist_ok=True)
        
    def init_camera(self):
        """初始化摄像头"""
        try:
            # 尝试初始化摄像头
            self.camera = cv2.VideoCapture(0)
            if not self.camera.isOpened():
                print("错误: 无法打开摄像头")
                return False
                
            # 设置摄像头参数
            self.camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.camera.set(cv2.CAP_PROP_FPS, 30)
            
            print("摄像头初始化成功")
            return True
        except Exception as e:
            print(f"摄像头初始化失败: {e}")
            return False
    
    def cleanup(self):
        """清理资源"""
        self.stop_stream()
        if self.camera:
            self.camera.release()
            self.camera = None
        print("摄像头资源清理完成")
    
    def is_available(self):
        """检查摄像头是否可用"""
        if self.camera is None:
            return self.init_camera()
        return self.camera.isOpened()
    
    def take_snapshot(self):
        """拍摄快照"""
        if not self.is_available():
            return False, "摄像头不可用"
            
        try:
            ret, frame = self.camera.read()
            if not ret:
                return False, "无法捕获图像"
                
            # 保存快照
            success = cv2.imwrite(self.snapshot_path, frame)
            if success:
                self.frame_count += 1
                self.last_frame_time = time.time()
                print(f"快照已保存: {self.snapshot_path}")
                return True, self.snapshot_path
            else:
                return False, "保存图像失败"
                
        except Exception as e:
            return False, f"拍摄快照时出错: {e}"
    
    def start_stream(self):
        """开始视频流"""
        if self.is_streaming:
            return True, "视频流已在运行"
            
        if not self.is_available():
            return False, "摄像头不可用"
            
        self.is_streaming = True
        self.stream_thread = threading.Thread(target=self._stream_worker)
        self.stream_thread.daemon = True
        self.stream_thread.start()
        
        print("视频流已开始")
        return True, "视频流已开始"
    
    def stop_stream(self):
        """停止视频流"""
        if not self.is_streaming:
            return True, "视频流未运行"
            
        self.is_streaming = False
        if self.stream_thread:
            self.stream_thread.join(timeout=2)
            
        print("视频流已停止")
        return True, "视频流已停止"
    
    def _stream_worker(self):
        """视频流工作线程"""
        print("视频流线程启动")
        
        while self.is_streaming:
            try:
                if not self.camera or not self.camera.isOpened():
                    break
                    
                ret, frame = self.camera.read()
                if not ret:
                    print("无法读取摄像头数据")
                    break
                    
                # 保存当前帧
                cv2.imwrite(self.stream_path, frame)
                self.frame_count += 1
                self.last_frame_time = time.time()
                
                # 控制帧率
                time.sleep(1/30)  # 30 FPS
                
            except Exception as e:
                print(f"视频流错误: {e}")
                break
                
        print("视频流线程结束")
    
    def get_status(self):
        """获取摄像头状态"""
        status = {
            "available": self.is_available(),
            "streaming": self.is_streaming,
            "frame_count": self.frame_count,
            "last_frame_time": self.last_frame_time,
            "resolution": "640x480",
            "fps": 30
        }
        
        if self.is_available():
            status["status"] = "running" if self.is_streaming else "ready"
        else:
            status["status"] = "error"
            
        return status

def main():
    """主函数 - 处理命令行参数"""
    if len(sys.argv) < 2:
        print("用法: python3 camera.py <command> [args]")
        print("命令:")
        print("  init        - 初始化摄像头")
        print("  snapshot    - 拍摄快照")
        print("  start_stream - 开始视频流")
        print("  stop_stream  - 停止视频流")
        print("  status      - 获取状态")
        print("  cleanup     - 清理资源")
        sys.exit(1)
    
    command = sys.argv[1]
    camera = CameraController()
    
    # 处理不同的命令
    if command == "init":
        success = camera.init_camera()
        result = {"success": success, "message": "摄像头初始化成功" if success else "摄像头初始化失败"}
        
    elif command == "snapshot":
        success, message = camera.take_snapshot()
        result = {
            "success": success,
            "message": message,
            "image_path": "/images/snapshot.jpg" if success else None
        }
        
    elif command == "start_stream":
        success, message = camera.start_stream()
        result = {
            "success": success,
            "message": message,
            "stream_path": "/images/stream.jpg" if success else None
        }
        
    elif command == "stop_stream":
        success, message = camera.stop_stream()
        result = {"success": success, "message": message}
        
    elif command == "status":
        status = camera.get_status()
        result = {"success": True, "data": status}
        
    elif command == "cleanup":
        camera.cleanup()
        result = {"success": True, "message": "摄像头资源清理完成"}
        
    else:
        result = {"success": False, "message": f"未知命令: {command}"}
    
    # 输出JSON结果
    print(json.dumps(result))
    
    # 如果是流式传输，保持程序运行
    if command == "start_stream" and result["success"]:
        try:
            while camera.is_streaming:
                time.sleep(1)
        except KeyboardInterrupt:
            camera.cleanup()

if __name__ == "__main__":
    main()