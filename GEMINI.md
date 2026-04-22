# 仓库文件结构描述

```text
.
├── GEMINI.md                    # 本文件，存放Agent指令及仓库结构描述
├── CMakeLists.txt               # 项目CMake构建配置文件
├── README.md                    # 项目自述文件，包含整体架构描述及环境准备说明
├── example_classes.txt          # 视觉识别模块的示例类别文件
├── test_vision_detection.sh     # 视觉识别功能测试脚本
├── build/                       # 编译构建目录（通常在.gitignore中忽略）
├── docs/                        # 详细模块文档及使用说明目录
│   ├── LineProcessor.md         # LineProcessor模块的使用示例与参数配置
│   └── vision_detection.md      # 视觉识别模块的详细文档
├── include/                     # 头文件目录
│   ├── LineProcessor.hpp
│   └── ONNXDetector.hpp         # ONNX模型推理器头文件
├── src/                         # 源文件目录
│   ├── LineProcessor.cpp
│   ├── ONNXDetector.cpp         # ONNX模型推理器实现
│   ├── go2_video_client.cpp     # 主程序入口，基于Unitree SDK2的视频客户端示例
│   ├── go2_sport_interactive.cpp # 交互式运动控制客户端，支持Stretch和Hello
│   └── go2_vision_detection.cpp # 视觉识别主程序，集成ONNX推理和机器人动作控制
└── unitree_sdk2/                # Unitree SDK2库及头文件
    ├── build/                   # Build artifacts (binaries, CMake cache)
    ├── cmake/                   # CMake configuration files
    ├── CMakeLists.txt           # Project CMake build configuration
    ├── example/                 # Example code for different robot models and features
    │   ├── go2/                 # Go2 robot examples
    │   ├── g1/                  # G1 robot examples
    │   ├── h1/                  # H1 robot examples
    │   ├── a2/                  # A2 robot examples
    │   ├── b2/                  # B2 robot examples
    │   └── ...
    ├── include/                 # Header files
    │   └── unitree/             # Core SDK headers
    ├── lib/                     # Compiled libraries
    ├── LICENSE                  # License information
    ├── README.md                # Original SDK README
    └── thirdparty/              # Third-party dependencies
```

## 新增功能模块：视觉识别

### 功能概述
新增的视觉识别模块 (`go2_vision_detection`) 实现了基于ONNX模型的实时安全标识检测功能：

1. **ONNX模型加载**：支持加载预训练的ONNX模型进行目标检测
2. **视频流处理**：实时获取Go2机器人相机视频流
3. **图像预处理**：使用LineProcessor进行图像增强和预处理
4. **可视化检测框**：自动在检测到的安全图标周围绘制彩色边界框和标签
5. **智能动作触发**：根据检测结果自动触发机器人动作：
   - `caution_shock` → Go2伸懒腰 (Stretch) [红色边框]
   - `caution_oxidizer` → Go2打招呼 (Hello) [黄色边框]
   - `caution_radiation` → Go2闪烁前灯三次 [蓝色边框]

### 技术特性
- **C++17标准**：使用现代C++特性
- **OpenCV DNN**：利用OpenCV的深度学习模块进行ONNX推理
- **多线程安全**：使用原子变量和互斥锁确保线程安全
- **可配置参数**：支持调整置信度阈值、NMS阈值等参数
- **可视化反馈**：实时显示检测结果、置信度和彩色边界框
- **完整检测框**：自动绘制目标边界框、类别标签和置信度显示

### 编译目标
- `go2_vision_detection`：视觉识别主程序
- `ONNXDetector`：ONNX推理器静态库
- `LineProcessor`：图像处理器静态库

### 使用示例
```bash
# 编译
mkdir build && cd build
cmake ..
make go2_vision_detection

# 运行
./go2_vision_detection safety_signs.onnx eth0
```

### 依赖要求
- OpenCV 4.x (需包含DNN模块)
- Unitree SDK2
- C++17兼容编译器
- ONNX模型文件（需预先训练）

### 文档位置
详细使用说明请参考：`docs/vision_detection.md`