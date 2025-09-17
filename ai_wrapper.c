/*
 * AI推理C封装模块
 * 将Python AI推理功能封装为C可调用的接口
 */

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// AI推理结果结构
typedef struct {
    int success;           // 是否成功
    int class_id;          // 预测类别ID
    char class_name[32];   // 类别名称
    float confidence;      // 置信度
    int meets_threshold;   // 是否满足置信度阈值
    char error_msg[256];   // 错误信息
} ai_result_t;

// 全局Python对象
static PyObject *g_ai_module = NULL;
static PyObject *g_ai_init_func = NULL;
static PyObject *g_ai_predict_func = NULL;
static PyObject *g_ai_cleanup_func = NULL;
static int g_python_initialized = 0;

// 初始化Python解释器和AI模块
int ai_wrapper_init(const char* model_path) {
    printf("初始化AI封装模块...\n");
    
    // 初始化Python解释器
    if (!Py_IsInitialized()) {
        Py_Initialize();
        if (!Py_IsInitialized()) {
            printf("错误: Python解释器初始化失败\n");
            return 0;
        }
        g_python_initialized = 1;
    }
    
    // 添加当前目录到Python路径
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('.')");
    
    // 导入AI推理模块
    g_ai_module = PyImport_ImportModule("ai_inference");
    if (!g_ai_module) {
        PyErr_Print();
        printf("错误: 无法导入ai_inference模块\n");
        return 0;
    }
    
    // 获取函数引用
    g_ai_init_func = PyObject_GetAttrString(g_ai_module, "ai_init");
    g_ai_predict_func = PyObject_GetAttrString(g_ai_module, "ai_predict_from_image");
    g_ai_cleanup_func = PyObject_GetAttrString(g_ai_module, "ai_cleanup");
    
    if (!g_ai_init_func || !g_ai_predict_func || !g_ai_cleanup_func) {
        PyErr_Print();
        printf("错误: 无法获取AI函数引用\n");
        return 0;
    }
    
    // 调用Python AI初始化函数
    PyObject *model_path_str = PyUnicode_FromString(model_path ? model_path : "model/line_follower.tflite");
    PyObject *init_args = PyTuple_Pack(1, model_path_str);
    PyObject *result = PyObject_CallObject(g_ai_init_func, init_args);
    
    int success = 0;
    if (result && PyBool_Check(result)) {
        success = (result == Py_True);
    }
    
    // 清理临时对象
    Py_XDECREF(model_path_str);
    Py_XDECREF(init_args);
    Py_XDECREF(result);
    
    if (success) {
        printf("AI模块初始化成功\n");
    } else {
        printf("AI模块初始化失败\n");
    }
    
    return success;
}

// 从图像数据进行AI推理
int ai_wrapper_predict(const unsigned char* image_data, int width, int height, 
                      float confidence_threshold, ai_result_t* result) {
    if (!g_ai_predict_func || !result) {
        return 0;
    }
    
    // 初始化结果结构
    memset(result, 0, sizeof(ai_result_t));
    
    // 创建Python字节对象
    Py_ssize_t data_size = width * height * 3; // BGR图像
    PyObject *image_bytes = PyBytes_FromStringAndSize((const char*)image_data, data_size);
    if (!image_bytes) {
        strcpy(result->error_msg, "创建图像字节对象失败");
        return 0;
    }
    
    // 创建参数元组
    PyObject *width_obj = PyLong_FromLong(width);
    PyObject *height_obj = PyLong_FromLong(height);
    PyObject *threshold_obj = PyFloat_FromDouble(confidence_threshold);
    PyObject *args = PyTuple_Pack(4, image_bytes, width_obj, height_obj, threshold_obj);
    
    // 调用Python预测函数
    PyObject *py_result = PyObject_CallObject(g_ai_predict_func, args);
    
    // 清理参数对象
    Py_DECREF(image_bytes);
    Py_DECREF(width_obj);
    Py_DECREF(height_obj);
    Py_DECREF(threshold_obj);
    Py_DECREF(args);
    
    if (!py_result) {
        PyErr_Print();
        strcpy(result->error_msg, "Python预测函数调用失败");
        return 0;
    }
    
    // 解析返回的字典结果
    if (PyDict_Check(py_result)) {
        // 获取success字段
        PyObject *success_obj = PyDict_GetItemString(py_result, "success");
        if (success_obj && PyBool_Check(success_obj)) {
            result->success = (success_obj == Py_True);
        }
        
        if (result->success) {
            // 获取class_id
            PyObject *class_id_obj = PyDict_GetItemString(py_result, "class_id");
            if (class_id_obj && PyLong_Check(class_id_obj)) {
                result->class_id = (int)PyLong_AsLong(class_id_obj);
            }
            
            // 获取class_name
            PyObject *class_name_obj = PyDict_GetItemString(py_result, "class_name");
            if (class_name_obj && PyUnicode_Check(class_name_obj)) {
                const char *name = PyUnicode_AsUTF8(class_name_obj);
                if (name) {
                    strncpy(result->class_name, name, sizeof(result->class_name) - 1);
                    result->class_name[sizeof(result->class_name) - 1] = '\0';
                }
            }
            
            // 获取confidence
            PyObject *confidence_obj = PyDict_GetItemString(py_result, "confidence");
            if (confidence_obj && PyFloat_Check(confidence_obj)) {
                result->confidence = (float)PyFloat_AsDouble(confidence_obj);
            }
            
            // 获取meets_threshold
            PyObject *threshold_obj = PyDict_GetItemString(py_result, "meets_threshold");
            if (threshold_obj && PyBool_Check(threshold_obj)) {
                result->meets_threshold = (threshold_obj == Py_True);
            }
        } else {
            // 获取错误信息
            PyObject *error_obj = PyDict_GetItemString(py_result, "error");
            if (error_obj && PyUnicode_Check(error_obj)) {
                const char *error = PyUnicode_AsUTF8(error_obj);
                if (error) {
                    strncpy(result->error_msg, error, sizeof(result->error_msg) - 1);
                    result->error_msg[sizeof(result->error_msg) - 1] = '\0';
                }
            }
        }
    }
    
    Py_DECREF(py_result);
    return result->success;
}

// 清理AI封装模块
void ai_wrapper_cleanup(void) {
    printf("清理AI封装模块...\n");
    
    // 调用Python清理函数
    if (g_ai_cleanup_func) {
        PyObject *result = PyObject_CallObject(g_ai_cleanup_func, NULL);
        Py_XDECREF(result);
    }
    
    // 清理Python对象引用
    Py_XDECREF(g_ai_init_func);
    Py_XDECREF(g_ai_predict_func);
    Py_XDECREF(g_ai_cleanup_func);
    Py_XDECREF(g_ai_module);
    
    g_ai_init_func = NULL;
    g_ai_predict_func = NULL;
    g_ai_cleanup_func = NULL;
    g_ai_module = NULL;
    
    // 清理Python解释器
    if (g_python_initialized) {
        Py_Finalize();
        g_python_initialized = 0;
    }
}

// 获取类别对应的动作建议
const char* ai_get_action_name(int class_id) {
    switch (class_id) {
        case 0: return "left";
        case 1: return "forward";
        case 2: return "right";
        case 3: return "stop";
        default: return "unknown";
    }
}

// 打印AI结果信息 (调试用)
void ai_print_result(const ai_result_t* result) {
    if (!result) return;
    
    printf("AI推理结果:\n");
    printf("  成功: %s\n", result->success ? "是" : "否");
    
    if (result->success) {
        printf("  类别ID: %d\n", result->class_id);
        printf("  类别名称: %s\n", result->class_name);
        printf("  置信度: %.3f\n", result->confidence);
        printf("  满足阈值: %s\n", result->meets_threshold ? "是" : "否");
        printf("  建议动作: %s\n", ai_get_action_name(result->class_id));
    } else {
        printf("  错误: %s\n", result->error_msg);
    }
}
