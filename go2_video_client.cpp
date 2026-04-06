#include <unitree/robot/go2/video/video_client.hpp>
#include <opencv2/opencv.hpp> // 引入 OpenCV 头文件
#include <iostream>

int main()
{
    // 1. 初始化
    unitree::robot::ChannelFactory::Instance()->Init(0);
    unitree::robot::go2::VideoClient video_client;
    video_client.SetTimeout(1.0f);
    video_client.Init();

    std::vector<uint8_t> image_sample;
    int ret;

    // 创建一个名为 "Go2 Real-time Video" 的窗口
    cv::namedWindow("Go2 Real-time Video", cv::WINDOW_AUTOSIZE);

    while (true)
    {
        // 2. 获取图像数据
        ret = video_client.GetImageSample(image_sample);

        if (ret == 0 && !image_sample.empty()) {
            // 3. 解码：将字节向量转换为 OpenCV 的 Mat 格式
            // CV_8UC3 表示 8位无符号 3通道（RGB）
            cv::Mat rawData(image_sample);
            cv::Mat frame = cv::imdecode(rawData, cv::IMREAD_COLOR);

            if (!frame.empty()) {
                // 4. 渲染显示
                cv::imshow("Go2 Real-time Video", frame);
            }
        }

        // 5. 关键：控制渲染频率并处理键盘事件（1ms 等待）
        // 如果按下 'q' 键或 Esc 键，退出循环
        char key = (char)cv::waitKey(1);
        if (key == 'q' || key == 27) {
            break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}