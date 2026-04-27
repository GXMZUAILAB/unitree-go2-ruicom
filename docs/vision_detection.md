# Go2 视觉识别功能文档

## 功能概述

本模块实现了基于ONNX模型的视觉识别功能，能够：
1. 加载预训练的ONNX模型进行安全标识检测
2. 实时获取Go2机器人相机视频流
3. 使用LineProcessor进行图像预处理
4. **自动绘制检测框**：在检测到的安全图标周围绘制彩色边界框
5. 根据检测结果触发相应的机器人动作：
   - 检测到 `caution_shock`（电击警告） → Go2伸懒腰
   - 检测到 `caution_oxidizer`（氧化剂警告） → Go2打招呼
   - 检测到 `caution_radiation`（辐射警告） → Go2闪烁前灯三次

## 系统要求

- C++17 编译器
- OpenCV 4.x (需包含DNN模块)
- Unitree SDK2
- ONNX Runtime 或 OpenCV DNN模块支持ONNX

## 文件结构

```
src/go2_vision_detection.cpp    # 主程序
include/ONNXDetector.hpp        # ONNX推理器头文件
src/ONNXDetector.cpp            # ONNX推理器实现
```

## 编译

```bash
mkdir build && cd build
cmake ..
make go2_vision_detection
```

## 使用方法

### 基本使用

```bash
# 1. 只加载ONNX模型（使用默认类名）
./go2_vision_detection path/to/model.onnx

# 2. 加载ONNX模型和类名文件
./go2_vision_detection path/to/model.onnx path/to/classes.txt

# 3. 加载ONNX模型、类名文件，并指定网络接口
./go2_vision_detection path/to/model.onnx path/to/classes.txt eth0

# 4. 加载ONNX模型并指定网络接口（使用默认类名）
./go2_vision_detection path/to/model.onnx eth0
```

### 参数说明

1. `onnx_model_path` (必需): ONNX模型文件路径
2. `classes_file` (可选): 类名文件路径，包含类别名称列表
3. `network_interface` (可选): 网络接口名称，如 `eth0`, `wlan0` 等

**注意**：当只有两个参数时，程序会自动检测第二个参数是类名文件还是网络接口：
- 如果第二个参数包含 `.`（如 `classes.txt`）→ 视为类名文件
- 如果第二个参数类似 `eth0`, `wlan0`, `enp0s3` → 视为网络接口

### 模型要求

ONNX模型应输出YOLO格式的检测结果：
- 输入尺寸: 640x640 (可通过修改代码调整)
- 输出格式: [batch, num_detections, 85]
  - 85 = [x, y, w, h, confidence, class_probabilities...]

### 类名文件 (`classes.txt`) 的作用

类名文件是连接ONNX模型输出和可读类别名称的关键桥梁：

#### 1. **为什么需要类名文件？**
- ONNX模型输出的是数字类别ID（0, 1, 2, ...）
- 这些数字ID需要映射到有意义的名称（如 `caution_shock`）
- 类名文件定义了这种映射关系

#### 2. **文件格式**
每行一个类别名称，顺序必须与模型训练时的类别顺序一致：
```
caution_shock        # 第0类
caution_oxidizer     # 第1类  
caution_radiation    # 第2类
caution_corrosive    # 第3类
caution_flammable    # 第4类
caution_toxic        # 第5类
```

#### 3. **映射关系**
```
模型输出ID → 类名文件行号 → 类别名称
     0     →      0      → caution_shock
     1     →      1      → caution_oxidizer
     2     →      2      → caution_radiation
     ...   →     ...     → ...
```

#### 4. **如果不提供类名文件**
- 程序会使用默认的安全标识类名列表
- 默认列表包含3个常见的安全标识类别
- 如果您的模型类别不同，必须提供正确的类名文件

#### 5. **创建类名文件**
```bash
# 示例：创建 classes.txt
echo "caution_shock" > classes.txt
echo "caution_oxidizer" >> classes.txt
echo "caution_radiation" >> classes.txt
echo "caution_corrosive" >> classes.txt
echo "caution_flammable" >> classes.txt
echo "caution_toxic" >> classes.txt

# 或者使用文本编辑器创建
```

#### 6. **验证类名加载**
程序启动时会显示加载的类别：
```
Go2 Vision Controller initialized successfully
Loaded classes: caution_shock caution_oxidizer caution_radiation ...
```

**重要**：确保类名文件的顺序与模型训练时的类别顺序完全一致！

## 程序流程

1. **初始化**
   - 初始化Unitree SDK网络通信
   - 加载ONNX模型
   - 初始化视频和运动客户端

2. **主循环**
   - 获取相机视频帧
   - 使用LineProcessor预处理图像
   - 使用ONNXDetector进行目标检测
   - **在图像上绘制检测框和标签**
   - 根据检测到的类别触发相应动作

3. **动作触发**
   - 每个动作有2秒冷却时间，防止重复触发
   - 只触发置信度最高的检测结果
   - 置信度阈值: 0.6 (可配置)

## 可视化效果

程序会显示一个名为 "Go2 Vision Detection" 的窗口，包含：

### 1. **边界框绘制**
- 每个检测到的目标周围绘制彩色矩形框
- 不同类别使用不同颜色：
  - `caution_shock`: 红色
  - `caution_oxidizer`: 黄色
  - `caution_radiation`: 蓝色
  - `first_mark`: 绿色
  - `second_mark`: 品红

### 2. **标签显示**
- 边界框上方显示类别名称和置信度百分比
- 图像左上角显示所有检测结果的汇总
- 标签背景使用与边界框相同的颜色

### 3. **视觉示例**
```
┌─────────────────────────────────────┐
│ caution_shock: 85%                  │ ← 顶部汇总标签
│ caution_oxidizer: 72%               │
│                                     │
│    ┌──────────────┐                 │
│    │  caution     │ ← 边界框+标签    │
│    │  shock: 85%  │                 │
│    │              │                 │
│    └──────────────┘                 │
│                                     │
└─────────────────────────────────────┘
```

## 配置参数

可以在 `ONNXDetector` 类中调整以下参数：

```cpp
// 置信度阈值 (默认0.6)
detector.setConfidenceThreshold(0.6f);

// NMS阈值 (默认0.4)
detector.setNMSThreshold(0.4f);

// 输入图像尺寸 (默认640x640)
// 在ONNXDetector构造函数中修改 inputSize_
```

## 动作说明

### 1. 伸懒腰 (Stretch)
- 触发条件: 检测到 `caution_shock`
- 实现: 调用 `sportClient_.Stretch()`
- 视觉反馈: 红色边界框

### 2. 打招呼 (Hello)
- 触发条件: 检测到 `caution_oxidizer`
- 实现: 调用 `sportClient_.Hello()`
- 视觉反馈: 黄色边界框

### 3. 闪烁前灯 (Flash Front Lights)
- 触发条件: 检测到 `caution_radiation`
- 实现: 使用Unitree SDK的VuiClient API控制前灯亮度，实现3次闪烁
  ```cpp
  // 闪烁3次，每次亮300ms，灭300ms
  for (int i = 0; i < 3; ++i) {
      vuiClient_.SetBrightness(10);  // 最大亮度
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      vuiClient_.SetBrightness(0);   // 关闭
      if (i < 2) std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
  ```
- 视觉反馈: 蓝色边界框
- 技术细节: 使用 `unitree::robot::go2::VuiClient` 的 `SetBrightness()` API，亮度范围0-10

## 按键控制

- `q` 或 `ESC`: 退出程序

## 数据结构

```cpp
struct DetectionResult {
    std::string className;  // 类别名称
    float confidence;       // 置信度 (0.0-1.0)
    cv::Rect boundingBox;   // 边界框坐标 (x, y, width, height)
};
```

## 故障排除

### 1. 模型加载失败
- 检查ONNX模型文件路径是否正确
- 确认OpenCV编译时启用了ONNX支持
- 检查模型输入输出格式是否符合要求

### 2. 视频流获取失败
- 检查网络连接
- 确认Unitree SDK正确安装
- 检查网络接口名称是否正确

### 3. 检测框不准确
- 检查模型训练质量
- 调整置信度阈值
- 验证模型输出格式是否正确

### 4. 动作执行失败
- 检查机器人是否处于正确模式
- 确认运动客户端初始化成功
- 检查网络延迟

## 扩展功能

### 添加新的检测类别
1. 在类名文件中添加新类别
2. 在 `handleDetections` 函数中添加对应的动作处理
3. 在 `drawDetections` 函数中添加新的颜色映射

### 调整检测参数
- 修改 `confidenceThreshold_` 调整检测灵敏度
- 修改 `detectionCooldown_` 调整动作触发频率
- 修改 `inputSize_` 适应不同尺寸的模型

### 增强可视化
- 添加置信度颜色渐变（高置信度用深色，低置信度用浅色）
- 添加检测框动画效果
- 添加历史检测轨迹显示

## 性能优化建议

1. **推理优化**
   - 使用GPU加速（如果OpenCV支持）
   - 调整输入图像尺寸减少计算量
   - 使用模型量化技术

2. **内存管理**
   - 及时释放不再使用的图像内存
   - 使用智能指针管理资源
   - 避免不必要的图像复制

3. **实时性保障**
   - 设置合适的帧率限制
   - 使用多线程处理推理和显示
   - 优化图像预处理流水线

## 注意事项

1. **模型质量**: 检测框准确性完全取决于ONNX模型的训练质量
2. **实时性**: 视频帧率可能受推理速度影响，复杂模型可能导致延迟
3. **安全性**: 确保动作触发不会导致机器人失控或碰撞
4. **网络稳定性**: 保持稳定的网络连接以确保视频流和运动控制
5. **光照条件**: 检测效果受环境光照影响，建议在良好光照条件下使用