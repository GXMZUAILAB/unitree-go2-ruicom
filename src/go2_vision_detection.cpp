#include <unitree/robot/go2/video/video_client.hpp>
#include <unitree/robot/go2/sport/sport_client.hpp>
#include <unitree/robot/go2/vui/vui_client.hpp>
#include <unitree/robot/channel/channel_factory.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

#include "LineProcessor.hpp"
#include "ONNXDetector.hpp"

class Go2VisionController {
public:
    Go2VisionController(const std::string& onnxModelPath,
                        const std::string& classesPath = "",
                        const std::string& netInterface = "")
        : isRunning_(false)
        , lastDetectionTime_(std::chrono::steady_clock::now())
        , detectionCooldown_(2000) // 2 seconds cooldown between actions
    {
        // Initialize network interface
        if (!netInterface.empty()) {
            unitree::robot::ChannelFactory::Instance()->Init(0, netInterface);
        } else {
            unitree::robot::ChannelFactory::Instance()->Init(0);
        }

        // Initialize video client
        videoClient_.SetTimeout(1.0f);
        videoClient_.Init();

        // Initialize sport client
        sportClient_.SetTimeout(10.0f);
        sportClient_.Init();

        // Initialize vui client for light control
        vuiClient_.SetTimeout(1.0f);
        vuiClient_.Init();

        // Load ONNX model with optional class names file
        if (!detector_.loadModel(onnxModelPath, classesPath)) {
            throw std::runtime_error("Failed to load ONNX model: " + onnxModelPath);
        }

        // Set detection thresholds
        detector_.setConfidenceThreshold(0.6f);
        detector_.setNMSThreshold(0.4f);

        std::cout << "Go2 Vision Controller initialized successfully" << std::endl;
        std::cout << "Loaded classes: ";
        auto classes = detector_.getClassNames();
        for (const auto& className : classes) {
            std::cout << className << " ";
        }
        std::cout << std::endl;
    }

    void run() {
        isRunning_ = true;
        cv::namedWindow("Go2 Vision Detection", cv::WINDOW_AUTOSIZE);
        std::cout << "Vision detection started. Press 'q' to quit." << std::endl;

        std::vector<uint8_t> imageSample;
        LineProcessor lineProcessor;

        while (isRunning_) {
            int ret = videoClient_.GetImageSample(imageSample);

            if (ret == 0 && !imageSample.empty()) {
                cv::Mat rawData(imageSample);
                cv::Mat frame = cv::imdecode(rawData, cv::IMREAD_COLOR);

                if (!frame.empty()) {
                    // Process frame with LineProcessor
                    cv::Mat processedFrame = lineProcessor.process(imageSample);

                    // Detect safety signs
                    auto detections = detector_.detect(frame);

                    // Draw detections on frame (with bounding boxes)
                    detector_.drawDetections(frame, detections);

                    // Display frame
                    cv::imshow("Go2 Vision Detection", frame);

                    // Handle detections and trigger actions
                    handleDetections(detections);

                    // Check for quit key
                    char key = (char)cv::waitKey(1);
                    if (key == 'q' || key == 27) {
                        stop();
                    }
                }
            } else {
                // Check for quit key even when no frame
                if ((char)cv::waitKey(1) == 'q') {
                    stop();
                }
            }
        }

        cv::destroyAllWindows();
    }

    void stop() {
        isRunning_ = false;
    }

private:
    void handleDetections(const std::vector<DetectionResult>& detections) {
        if (detections.empty()) {
            return;
        }

        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastDetection = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastDetectionTime_).count();

        // Apply cooldown to prevent rapid repeated actions
        if (timeSinceLastDetection < detectionCooldown_) {
            return;
        }

        // Find the detection with highest confidence
        std::string highestClass;
        float highestConfidence = 0.0f;

        for (const auto& detection : detections) {
            if (detection.confidence > highestConfidence) {
                highestConfidence = detection.confidence;
                highestClass = detection.className;
            }
        }

        // Trigger action based on detected class
        if (highestConfidence > 0.6f) {
            std::cout << "Detected: " << highestClass << " with confidence: "
                      << (highestConfidence * 100) << "%" << std::endl;

            if (highestClass == "caution_shock") {
                performStretch();
                lastDetectionTime_ = now;
            } else if (highestClass == "caution_oxidizer") {
                performHello();
                lastDetectionTime_ = now;
            } else if (highestClass == "caution_radiation") {
                flashFrontLights();
                lastDetectionTime_ = now;
            }
        }
    }

    void performStretch() {
        std::cout << ">>> Performing Stretch (伸懒腰) action" << std::endl;
        try {
            sportClient_.Stretch();
            std::cout << ">>> Stretch action completed" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "!!! Failed to perform Stretch: " << e.what() << std::endl;
        }
    }

    void performHello() {
        std::cout << ">>> Performing Hello (打招呼) action" << std::endl;
        try {
            sportClient_.Hello();
            std::cout << ">>> Hello action completed" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "!!! Failed to perform Hello: " << e.what() << std::endl;
        }
    }

    void flashFrontLights() {
        std::cout << ">>> Flashing front lights 3 times" << std::endl;

        try {
            // Flash lights 3 times
            for (int i = 0; i < 3; ++i) {
                // Turn lights on (brightness level 10 - maximum)
                int ret = vuiClient_.SetBrightness(10);
                if (ret != 0) {
                    std::cerr << "!!! Failed to turn lights on: error code " << ret << std::endl;
                } else {
                    std::cout << "   Flash " << (i + 1) << " - Lights ON" << std::endl;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(300));

                // Turn lights off (brightness level 0)
                ret = vuiClient_.SetBrightness(0);
                if (ret != 0) {
                    std::cerr << "!!! Failed to turn lights off: error code " << ret << std::endl;
                } else {
                    std::cout << "   Flash " << (i + 1) << " - Lights OFF" << std::endl;
                }

                // Wait between flashes (except after last one)
                if (i < 2) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                }
            }

            std::cout << ">>> Front lights flashing completed" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "!!! Failed to flash lights: " << e.what() << std::endl;
        }
    }

private:
    unitree::robot::go2::VideoClient videoClient_;
    unitree::robot::go2::SportClient sportClient_;
    unitree::robot::go2::VuiClient vuiClient_;
    ONNXDetector detector_;

    std::atomic<bool> isRunning_;
    std::chrono::steady_clock::time_point lastDetectionTime_;
    const int detectionCooldown_; // milliseconds

    std::mutex actionMutex_;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <onnx_model_path> [classes_file] [network_interface]" << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  1. onnx_model_path  : Path to ONNX model file (required)" << std::endl;
        std::cout << "  2. classes_file     : Path to class names text file (optional)" << std::endl;
        std::cout << "  3. network_interface: Network interface name, e.g., eth0, wlan0 (optional)" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " safety_signs.onnx classes.txt eth0" << std::endl;
        std::cout << "  " << argv[0] << " safety_signs.onnx classes.txt" << std::endl;
        std::cout << "  " << argv[0] << " safety_signs.onnx" << std::endl;
        return -1;
    }

    std::string onnxModelPath = argv[1];
    std::string classesPath = (argc > 2) ? argv[2] : "";
    std::string netInterface = (argc > 3) ? argv[3] : "";

    // Check if the third argument might be a network interface (not a file)
    if (argc == 3) {
        // Check if the second argument looks like a network interface, not a file
        std::string arg2 = argv[2];
        if (arg2.find('.') == std::string::npos &&
            (arg2.find("eth") == 0 || arg2.find("wlan") == 0 || arg2.find("enp") == 0)) {
            // Likely a network interface, not a classes file
            classesPath = "";
            netInterface = arg2;
        }
    }

    try {
        Go2VisionController controller(onnxModelPath, classesPath, netInterface);
        controller.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}