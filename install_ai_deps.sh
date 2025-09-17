#!/bin/bash

# AI推理环境安装脚本
# 为树莓派设置TensorFlow Lite和相关依赖

echo "=== 树莓派AI推理环境安装脚本 ==="

# 检查Python版本
PYTHON_VERSION=$(python3 --version 2>&1 | grep -oP '\d+\.\d+')
echo "检测到Python版本: $PYTHON_VERSION"

# 更新系统包
echo "更新系统包..."
sudo apt update

# 安装基础依赖
echo "安装基础依赖..."
sudo apt install -y python3-pip python3-dev python3-venv
sudo apt install -y libopencv-dev python3-opencv
sudo apt install -y pkg-config cmake build-essential
sudo apt install -y libatlas-base-dev libhdf5-dev libhdf5-serial-dev
sudo apt install -y libjpeg-dev libpng-dev libtiff-dev
sudo apt install -y libavcodec-dev libavformat-dev libswscale-dev
sudo apt install -y libv4l-dev libxvidcore-dev libx264-dev
sudo apt install -y libgtk-3-dev libcanberra-gtk-module libcanberra-gtk3-module
sudo apt install -y libqtgui4 libqtwebkit4 libqt4-test python3-pyqt5

# 安装Python包管理工具
echo "升级pip..."
python3 -m pip install --upgrade pip

# 安装TensorFlow Lite
echo "安装TensorFlow Lite Runtime..."
if [[ "$PYTHON_VERSION" == "3.9" ]]; then
    python3 -m pip install https://github.com/google-coral/pycoral/releases/download/v2.0.0/tflite_runtime-2.5.0.post1-cp39-cp39-linux_armv7l.whl
elif [[ "$PYTHON_VERSION" == "3.8" ]]; then
    python3 -m pip install https://github.com/google-coral/pycoral/releases/download/v2.0.0/tflite_runtime-2.5.0.post1-cp38-cp38-linux_armv7l.whl
elif [[ "$PYTHON_VERSION" == "3.7" ]]; then
    python3 -m pip install https://github.com/google-coral/pycoral/releases/download/v2.0.0/tflite_runtime-2.5.0.post1-cp37-cp37m-linux_armv7l.whl
else
    echo "尝试通过pip安装tflite-runtime..."
    python3 -m pip install tflite-runtime
fi

# 安装其他Python依赖
echo "安装Python依赖包..."
python3 -m pip install numpy opencv-python-headless

# 验证安装
echo "验证安装..."
python3 -c "
try:
    import tflite_runtime.interpreter as tflite
    print('✓ TensorFlow Lite Runtime 安装成功')
except ImportError:
    try:
        import tensorflow.lite as tflite
        print('✓ TensorFlow Lite 安装成功')
    except ImportError:
        print('✗ TensorFlow Lite 安装失败')
        exit(1)

try:
    import cv2
    print('✓ OpenCV 安装成功')
except ImportError:
    print('✗ OpenCV 安装失败')
    exit(1)

try:
    import numpy as np
    print('✓ NumPy 安装成功')
except ImportError:
    print('✗ NumPy 安装失败')
    exit(1)

print('所有依赖安装验证完成！')
"

# 检查是否有TFLite模型文件
if [ ! -f "model/line_follower.tflite" ]; then
    echo ""
    echo "警告: 未找到模型文件 model/line_follower.tflite"
    echo "请确保您有训练好的TensorFlow Lite模型文件"
    echo ""
    
    # 创建模型目录
    mkdir -p model
    
    # 创建一个示例模型描述文件
    cat > model/README.md << EOF
# 线路跟踪模型

将您训练好的TensorFlow Lite模型文件放在此目录下，文件名为：line_follower.tflite

## 模型要求：
- 输入: 图像 (建议尺寸: 224x224x3)
- 输出: 4个类别的概率 [left, forward, right, stop]
- 格式: TensorFlow Lite (.tflite)

## 模型训练参考：
您可以使用TensorFlow/Keras训练模型，然后转换为TFLite格式：

\`\`\`python
# 转换模型示例
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

with open('line_follower.tflite', 'wb') as f:
    f.write(tflite_model)
\`\`\`
EOF
fi

echo ""
echo "=== 安装完成 ==="
echo "现在您可以编译和运行AI推理功能："
echo ""
echo "1. 编译项目:"
echo "   make"
echo ""
echo "2. 编译AI测试程序:"
echo "   make test_ai"
echo ""
echo "3. 运行AI测试程序:"
echo "   ./test_ai [模型文件路径]"
echo ""
echo "注意: 请确保您有训练好的TensorFlow Lite模型文件"
