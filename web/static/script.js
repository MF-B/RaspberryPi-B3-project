// API基础配置 - 动态获取当前页面的host
const API_BASE_URL = `${window.location.protocol}//${window.location.host}`;

// 全局状态
let autoRefresh = false;
let autoRefreshInterval = null;

// DOM元素
const elements = {
    connectionStatus: document.getElementById('connection-status'),
    lastUpdate: document.getElementById('last-update'),
    temperature: document.getElementById('temperature'),
    humidity: document.getElementById('humidity'),
    distance: document.getElementById('distance'),
    currentTime: document.getElementById('current-time'),
    refreshSensors: document.getElementById('refresh-sensors'),
    rgbCurrentStatus: document.getElementById('rgb-current-status'),
    rgbOff: document.getElementById('rgb-off'),
    beepDuration: document.getElementById('beep-duration'),
    durationDisplay: document.getElementById('duration-display'),
    beepShort: document.getElementById('beep-short'),
    beepMedium: document.getElementById('beep-medium'),
    beepLong: document.getElementById('beep-long'),
    beepCustom: document.getElementById('beep-custom'),
    serverStatus: document.getElementById('server-status'),
    lastSensorUpdate: document.getElementById('last-sensor-update'),
    connectionLatency: document.getElementById('connection-latency'),
    checkStatus: document.getElementById('check-status'),
    autoRefreshToggle: document.getElementById('auto-refresh-toggle'),
    loadingOverlay: document.getElementById('loading-overlay'),
    notification: document.getElementById('notification'),
    notificationMessage: document.getElementById('notification-message'),
    notificationClose: document.getElementById('notification-close'),
    // 运动控制元素
    motionForward: document.getElementById('motion-forward'),
    motionBackward: document.getElementById('motion-backward'),
    motionLeft: document.getElementById('motion-left'),
    motionRight: document.getElementById('motion-right'),
    motionStop: document.getElementById('motion-stop'),
    speedSlider: document.getElementById('speed-slider'),
    speedValue: document.getElementById('speed-value'),
    motionDirection: document.getElementById('motion-direction'),
    motionSpeed: document.getElementById('motion-speed'),
    motionState: document.getElementById('motion-state'),
    // 摄像头元素
    cameraPreview: document.getElementById('camera-preview'),
    cameraSnapshot: document.getElementById('camera-snapshot'),
    cameraStartStream: document.getElementById('camera-start-stream'),
    cameraStopStream: document.getElementById('camera-stop-stream'),
    cameraStatus: document.getElementById('camera-status'),
    cameraStreamingStatus: document.getElementById('camera-streaming-status'),
    cameraResolution: document.getElementById('camera-resolution'),
    cameraFrameCount: document.getElementById('camera-frame-count'),
    previewPlaceholder: document.getElementById('camera-placeholder')
};

// 工具函数
class Utils {
    static formatTime(date) {
        return date.toLocaleTimeString('zh-CN', {
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });
    }

    static formatDate(date) {
        return date.toLocaleString('zh-CN');
    }

    static showLoading() {
        elements.loadingOverlay.classList.add('show');
    }

    static hideLoading() {
        elements.loadingOverlay.classList.remove('show');
    }

    static showNotification(message, type = 'info') {
        elements.notificationMessage.textContent = message;
        elements.notification.className = `notification show ${type}`;
        
        // 3秒后自动隐藏
        setTimeout(() => {
            this.hideNotification();
        }, 3000);
    }

    static hideNotification() {
        elements.notification.classList.remove('show');
    }

    static updateConnectionStatus(online) {
        elements.connectionStatus.textContent = online ? '在线' : '离线';
        elements.connectionStatus.className = `status ${online ? 'online' : 'offline'}`;
        elements.lastUpdate.textContent = online ? 
            Utils.formatTime(new Date()) : '连接失败';
    }
}

// API客户端
class ApiClient {
    static async request(endpoint, options = {}) {
        const startTime = Date.now();
        
        try {
            const response = await fetch(`${API_BASE_URL}${endpoint}`, {
                ...options,
                headers: {
                    'Content-Type': 'application/json',
                    ...options.headers
                }
            });

            const latency = Date.now() - startTime;
            elements.connectionLatency.textContent = `${latency}ms`;

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }

            return await response.json();
        } catch (error) {
            elements.connectionLatency.textContent = '--ms';
            throw error;
        }
    }

    static async getStatus() {
        return await this.request('/api/status');
    }

    static async getSensors() {
        return await this.request('/api/sensors');
    }

    static async controlRgb(action, color = null) {
        console.log('ApiClient.controlRgb called:', { action, color });
        const body = { action };
        if (color) body.color = color;
        
        console.log('Request body:', JSON.stringify(body));
        
        try {
            const result = await this.request('/api/rgb', {
                method: 'POST',
                body: JSON.stringify(body)
            });
            console.log('RGB API response:', result);
            return result;
        } catch (error) {
            console.error('RGB API request failed:', error);
            throw error;
        }
    }

    static async controlBeep(duration) {
        return await this.request('/api/beep', {
            method: 'POST',
            body: JSON.stringify({
                action: 'beep',
                duration: duration
            })
        });
    }

    static async controlMotion(direction, speed) {
        return await this.request('/api/motion', {
            method: 'POST',
            body: JSON.stringify({ 
                action: direction,
                speed: speed 
            })
        });
    }

    static async getDistance() {
        return await this.request('/api/distance');
    }
}

// 传感器数据管理
class SensorManager {
    static async updateSensorData() {
        try {
            const data = await ApiClient.getSensors();
            
            if (data.sensors.temp.status === 'success') {
                elements.temperature.textContent = data.sensors.temp.temperature.toFixed(1);
                elements.humidity.textContent = data.sensors.temp.humidity.toFixed(1);
            } else {
                elements.temperature.textContent = '--';
                elements.humidity.textContent = '--';
            }

            if (data.sensors.distance.status === 'success') {
                elements.distance.textContent = data.sensors.distance.distance;
            } else {
                elements.distance.textContent = '--';
            }

            elements.lastSensorUpdate.textContent = Utils.formatTime(new Date());
            Utils.updateConnectionStatus(true);
            
            return true;
        } catch (error) {
            console.error('获取传感器数据失败:', error);
            elements.temperature.textContent = '--';
            elements.humidity.textContent = '--';
            elements.distance.textContent = '--';
            Utils.updateConnectionStatus(false);
            Utils.showNotification('传感器数据获取失败', 'error');
            return false;
        }
    }
}

// RGB LED控制
class RgbController {
    static async setColor(color) {
        console.log('RgbController.setColor called with:', color);
        try {
            Utils.showLoading();
            const result = await ApiClient.controlRgb('on', color);
            console.log('RGB setColor result:', result);
            
            if (result && result.status === 'success') {
                elements.rgbCurrentStatus.textContent = `${color}色`;
                Utils.showNotification(`RGB LED已设置为${color}色`, 'success');
            } else {
                console.warn('RGB API returned non-success status:', result);
                Utils.showNotification('RGB LED控制失败', 'error');
            }
        } catch (error) {
            console.error('RGB控制失败:', error);
            Utils.showNotification('RGB LED控制失败', 'error');
        } finally {
            Utils.hideLoading();
        }
    }

    static async turnOff() {
        try {
            Utils.showLoading();
            const result = await ApiClient.controlRgb('off');
            
            if (result.status === 'success') {
                elements.rgbCurrentStatus.textContent = '关闭';
                Utils.showNotification('RGB LED已关闭', 'success');
            }
        } catch (error) {
            console.error('RGB关闭失败:', error);
            Utils.showNotification('RGB LED关闭失败', 'error');
        } finally {
            Utils.hideLoading();
        }
    }
}

// 蜂鸣器控制
class BeepController {
    static async beep(duration) {
        try {
            Utils.showLoading();
            const result = await ApiClient.controlBeep(duration);
            
            if (result.status === 'success') {
                Utils.showNotification(`蜂鸣器响了${duration}毫秒`, 'success');
            }
        } catch (error) {
            console.error('蜂鸣器控制失败:', error);
            Utils.showNotification('蜂鸣器控制失败', 'error');
        } finally {
            Utils.hideLoading();
        }
    }
}

// 运动控制
class MotionController {
    static currentSpeed = 50;
    static isMoving = false;
    static currentDirection = 'stopped';

    static async forward() {
        return this.move('forward');
    }

    static async backward() {
        return this.move('backward');
    }

    static async left() {
        return this.move('left');
    }

    static async right() {
        return this.move('right');
    }

    static async stop() {
        return this.move('stop');
    }

    static async move(direction) {
        try {
            const result = await ApiClient.controlMotion(direction, this.currentSpeed);
            
            if (result.status === 'success') {
                this.currentDirection = direction;
                this.isMoving = direction !== 'stop';
                this.updateMotionStatus();
                
                const actionText = this.getActionText(direction);
                Utils.showNotification(actionText, 'success');
            }
            
            return result;
        } catch (error) {
            console.error('运动控制失败:', error);
            Utils.showNotification('运动控制失败', 'error');
            return null;
        }
    }

    static setSpeed(speed) {
        this.currentSpeed = parseInt(speed);
        if (elements.speedValue) {
            elements.speedValue.textContent = `${speed}%`;
        }
        this.updateMotionStatus();
    }

    static updateMotionStatus() {
        if (elements.motionDirection) {
            elements.motionDirection.textContent = this.getDirectionText(this.currentDirection);
        }
        if (elements.motionSpeed) {
            elements.motionSpeed.textContent = `${this.currentSpeed}%`;
        }
        if (elements.motionState) {
            elements.motionState.textContent = this.isMoving ? '运行中' : '停止';
        }
    }

    static getDirectionText(direction) {
        const directions = {
            'forward': '前进',
            'backward': '后退',
            'left': '左转',
            'right': '右转',
            'stop': '停止',
            'stopped': '停止'
        };
        return directions[direction] || '未知';
    }

    static getActionText(direction) {
        const actions = {
            'forward': '开始前进',
            'backward': '开始后退',
            'left': '开始左转',
            'right': '开始右转',
            'stop': '已停止运动'
        };
        return actions[direction] || '运动状态变更';
    }
}

// 系统状态管理
class SystemManager {
    static async checkStatus() {
        try {
            Utils.showLoading();
            const status = await ApiClient.getStatus();
            
            elements.serverStatus.textContent = status.status;
            Utils.updateConnectionStatus(true);
            Utils.showNotification('系统状态检查完成', 'success');
            
            return true;
        } catch (error) {
            console.error('状态检查失败:', error);
            elements.serverStatus.textContent = '离线';
            Utils.updateConnectionStatus(false);
            Utils.showNotification('系统状态检查失败', 'error');
            return false;
        } finally {
            Utils.hideLoading();
        }
    }

    static toggleAutoRefresh() {
        autoRefresh = !autoRefresh;
        
        if (autoRefresh) {
            autoRefreshInterval = setInterval(() => {
                SensorManager.updateSensorData();
            }, 5000); // 每5秒刷新一次
            
            elements.autoRefreshToggle.innerHTML = '<i class="fas fa-sync"></i> 自动刷新: 开启';
            elements.autoRefreshToggle.classList.remove('btn-success');
            elements.autoRefreshToggle.classList.add('btn-warning');
            Utils.showNotification('自动刷新已开启', 'info');
        } else {
            if (autoRefreshInterval) {
                clearInterval(autoRefreshInterval);
                autoRefreshInterval = null;
            }
            
            elements.autoRefreshToggle.innerHTML = '<i class="fas fa-sync"></i> 自动刷新: 关闭';
            elements.autoRefreshToggle.classList.remove('btn-warning');
            elements.autoRefreshToggle.classList.add('btn-success');
            Utils.showNotification('自动刷新已关闭', 'info');
        }
    }
}

// 事件监听器设置
function setupEventListeners() {
    // 传感器刷新
    elements.refreshSensors.addEventListener('click', () => {
        SensorManager.updateSensorData();
    });

    // RGB LED控制
    document.querySelectorAll('.color-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const color = e.target.closest('.color-btn').dataset.color;
            RgbController.setColor(color);
        });
    });

    elements.rgbOff.addEventListener('click', () => {
        RgbController.turnOff();
    });

    // 蜂鸣器控制
    elements.beepDuration.addEventListener('input', (e) => {
        elements.durationDisplay.textContent = `${e.target.value}ms`;
    });

    elements.beepShort.addEventListener('click', () => {
        BeepController.beep(200);
    });

    elements.beepMedium.addEventListener('click', () => {
        BeepController.beep(1000);
    });

    elements.beepLong.addEventListener('click', () => {
        BeepController.beep(2000);
    });

    elements.beepCustom.addEventListener('click', () => {
        const duration = parseInt(elements.beepDuration.value);
        BeepController.beep(duration);
    });

    // 系统控制
    elements.checkStatus.addEventListener('click', () => {
        SystemManager.checkStatus();
    });

    elements.autoRefreshToggle.addEventListener('click', () => {
        SystemManager.toggleAutoRefresh();
    });

    // 运动控制事件
    if (elements.motionForward) {
        elements.motionForward.addEventListener('click', () => {
            MotionController.forward();
        });
    }

    if (elements.motionBackward) {
        elements.motionBackward.addEventListener('click', () => {
            MotionController.backward();
        });
    }

    if (elements.motionLeft) {
        elements.motionLeft.addEventListener('click', () => {
            MotionController.left();
        });
    }

    if (elements.motionRight) {
        elements.motionRight.addEventListener('click', () => {
            MotionController.right();
        });
    }

    if (elements.motionStop) {
        elements.motionStop.addEventListener('click', () => {
            MotionController.stop();
        });
    }

    if (elements.speedSlider) {
        elements.speedSlider.addEventListener('input', (e) => {
            MotionController.setSpeed(e.target.value);
        });
    }

    // 摄像头控制事件
    if (elements.cameraSnapshot) {
        elements.cameraSnapshot.addEventListener('click', () => {
            CameraController.takeSnapshot();
        });
    }

    if (elements.cameraStartStream) {
        elements.cameraStartStream.addEventListener('click', () => {
            CameraController.startStream();
        });
    }

    if (elements.cameraStopStream) {
        elements.cameraStopStream.addEventListener('click', () => {
            CameraController.stopStream();
        });
    }

    // 通知关闭
    elements.notificationClose.addEventListener('click', () => {
        Utils.hideNotification();
    });

    // 键盘快捷键
    document.addEventListener('keydown', (e) => {
        if (e.ctrlKey) {
            switch (e.key) {
                case 'r':
                    e.preventDefault();
                    SensorManager.updateSensorData();
                    break;
                case 'q':
                    e.preventDefault();
                    SystemManager.checkStatus();
                    break;
            }
        } else {
            // 运动控制快捷键 (WASD + 空格)
            switch (e.key.toLowerCase()) {
                case 'w':
                case 'arrowup':
                    e.preventDefault();
                    MotionController.forward();
                    break;
                case 's':
                case 'arrowdown':
                    e.preventDefault();
                    MotionController.backward();
                    break;
                case 'a':
                case 'arrowleft':
                    e.preventDefault();
                    MotionController.left();
                    break;
                case 'd':
                case 'arrowright':
                    e.preventDefault();
                    MotionController.right();
                    break;
                case ' ':
                    e.preventDefault();
                    MotionController.stop();
                    break;
            }
        }
    });
}

// 时钟更新
function updateClock() {
    const now = new Date();
    elements.currentTime.textContent = Utils.formatTime(now);
}

// 初始化应用
async function initializeApp() {
    console.log('初始化树莓派B3硬件控制系统...');
    
    // 设置事件监听器
    setupEventListeners();
    
    // 启动时钟
    updateClock();
    setInterval(updateClock, 1000);
    
    // 初始状态检查
    Utils.showLoading();
    
    try {
        // 检查系统状态
        await SystemManager.checkStatus();
        
        // 获取初始传感器数据
        await SensorManager.updateSensorData();
        
        // 初始化运动控制状态
        MotionController.updateMotionStatus();
        if (elements.speedSlider) {
            elements.speedSlider.value = MotionController.currentSpeed;
            MotionController.setSpeed(MotionController.currentSpeed);
        }
        
        // 获取摄像头状态
        await CameraController.getStatus();
        
        Utils.showNotification('系统初始化完成', 'success');
    } catch (error) {
        console.error('初始化失败:', error);
        Utils.showNotification('系统初始化失败，请检查设备连接', 'error');
    } finally {
        Utils.hideLoading();
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', initializeApp);

// 页面卸载时清理
window.addEventListener('beforeunload', () => {
    if (autoRefreshInterval) {
        clearInterval(autoRefreshInterval);
    }
});

// 错误处理
window.addEventListener('error', (e) => {
    console.error('全局错误:', e.error);
    Utils.showNotification('系统发生错误，请刷新页面', 'error');
});

// 网络状态检测
window.addEventListener('online', () => {
    Utils.showNotification('网络连接已恢复', 'success');
    SystemManager.checkStatus();
});

window.addEventListener('offline', () => {
    Utils.updateConnectionStatus(false);
    Utils.showNotification('网络连接已断开', 'error');
});

// 摄像头控制
class CameraController {
    static streamInterval = null;
    static isStreaming = false;

    static async takeSnapshot() {
        console.log('开始执行拍照功能...');
        try {
            Utils.showLoading();
            
            console.log('发送拍照API请求...');
            const response = await fetch(`${API_BASE_URL}/api/camera`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    action: "snapshot"
                })
            });

            console.log('收到API响应，状态码:', response.status);
            const data = await response.json();
            console.log('API响应数据:', data);
            
            if (data.status === "success") {
                // 更新预览图片
                if (data.image_url) {
                    elements.cameraPreview.src = data.image_url + "?t=" + Date.now();
                    elements.cameraPreview.style.display = "block";
                    elements.previewPlaceholder.style.display = "none";
                }
                Utils.showNotification("拍照成功", "success");
            } else {
                Utils.showNotification(`拍照失败: ${data.message}`, "error");
            }
        } catch (error) {
            console.error("拍照失败:", error);
            Utils.showNotification("拍照失败，请检查网络连接", "error");
        } finally {
            Utils.hideLoading();
        }
    }

    static async startStream() {
        console.log('开始视频流...');
        try {
            Utils.showLoading();
            
            const response = await fetch(`${API_BASE_URL}/api/camera`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    action: "start_stream"
                })
            });

            const data = await response.json();
            console.log('启动视频流响应:', data);
            
            if (data.status === "success") {
                this.isStreaming = true;
                // 更新UI状态
                if (elements.cameraStartStream) elements.cameraStartStream.style.display = "none";
                if (elements.cameraStopStream) elements.cameraStopStream.style.display = "inline-block";
                if (elements.cameraStreamingStatus) elements.cameraStreamingStatus.textContent = "运行中";
                
                // 开始定期刷新实时画面
                this.startStreamRefresh();
                
                Utils.showNotification("视频流已启动", "success");
            } else {
                Utils.showNotification(`启动视频流失败: ${data.message}`, "error");
            }
        } catch (error) {
            console.error("启动视频流失败:", error);
            Utils.showNotification("启动视频流失败，请检查网络连接", "error");
        } finally {
            Utils.hideLoading();
        }
    }

    static async stopStream() {
        console.log('停止视频流...');
        try {
            Utils.showLoading();
            
            const response = await fetch(`${API_BASE_URL}/api/camera`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({
                    action: "stop_stream"
                })
            });

            const data = await response.json();
            console.log('停止视频流响应:', data);
            
            if (data.status === "success") {
                this.isStreaming = false;
                this.stopStreamRefresh();
                
                // 更新UI状态
                if (elements.cameraStartStream) elements.cameraStartStream.style.display = "inline-block";
                if (elements.cameraStopStream) elements.cameraStopStream.style.display = "none";
                if (elements.cameraStreamingStatus) elements.cameraStreamingStatus.textContent = "停止";
                
                Utils.showNotification("视频流已停止", "success");
            } else {
                Utils.showNotification(`停止视频流失败: ${data.message}`, "error");
            }
        } catch (error) {
            console.error("停止视频流失败:", error);
            Utils.showNotification("停止视频流失败，请检查网络连接", "error");
        } finally {
            Utils.hideLoading();
        }
    }

    static startStreamRefresh() {
        // 提高刷新频率到每100ms（约10fps显示），配合后端15fps的帧率
        this.streamInterval = setInterval(() => {
            if (this.isStreaming) {
                const timestamp = Date.now();
                const img = new Image();
                img.onload = () => {
                    elements.cameraPreview.src = img.src;
                    elements.cameraPreview.style.display = "block";
                    elements.previewPlaceholder.style.display = "none";
                };
                img.onerror = () => {
                    console.warn('加载实时图像失败，继续尝试...');
                };
                img.src = "/images/live_frame.jpg?t=" + timestamp;
            }
        }, 100); // 100ms间隔，约10fps显示
    }

    static stopStreamRefresh() {
        if (this.streamInterval) {
            clearInterval(this.streamInterval);
            this.streamInterval = null;
        }
    }

    static async getStatus() {
        try {
            const response = await fetch(`${API_BASE_URL}/api/camera`);
            const data = await response.json();
            
            if (data.status === "success" && data.camera) {
                // 更新状态显示
                if (elements.cameraStatus) {
                    elements.cameraStatus.textContent = data.camera.status || "未知";
                }
                if (elements.cameraStreamingStatus) {
                    elements.cameraStreamingStatus.textContent = data.camera.streaming ? "运行中" : "停止";
                }
                if (elements.cameraResolution) {
                    const config = data.camera.config;
                    const resolution = config ? `${config.width}x${config.height}` : "640x480";
                    elements.cameraResolution.textContent = resolution;
                }
                if (elements.cameraFrameCount) {
                    elements.cameraFrameCount.textContent = data.camera.frame_count || "0";
                }
                
                // 根据流状态更新UI
                const isStreaming = data.camera.streaming;
                if (isStreaming !== this.isStreaming) {
                    this.isStreaming = isStreaming;
                    if (isStreaming) {
                        if (elements.cameraStartStream) elements.cameraStartStream.style.display = "none";
                        if (elements.cameraStopStream) elements.cameraStopStream.style.display = "inline-block";
                        this.startStreamRefresh();
                    } else {
                        if (elements.cameraStartStream) elements.cameraStartStream.style.display = "inline-block";
                        if (elements.cameraStopStream) elements.cameraStopStream.style.display = "none";
                        this.stopStreamRefresh();
                    }
                }
            }
            return data;
        } catch (error) {
            console.error("获取摄像头状态失败:", error);
            return null;
        }
    }
}
