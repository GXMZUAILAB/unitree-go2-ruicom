#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>
#include <memory>

struct DetectionResult {
    std::string className;
    float confidence;
    cv::Rect boundingBox;

    DetectionResult(const std::string& name, float conf, const cv::Rect& box)
        : className(name), confidence(conf), boundingBox(box) {}
};

class ONNXDetector {
public:
    ONNXDetector();
    ~ONNXDetector();

    bool loadModel(const std::string& modelPath, const std::string& classesPath = "");

    std::vector<DetectionResult> detect(const cv::Mat& image);

    void setConfidenceThreshold(float threshold);
    void setNMSThreshold(float threshold);

    std::vector<std::string> getClassNames() const;

    void drawDetections(cv::Mat& image, const std::vector<DetectionResult>& detections);

private:
    cv::dnn::Net net_;
    std::vector<std::string> classNames_;

    float confidenceThreshold_;
    float nmsThreshold_;

    cv::Size inputSize_;
    float scaleFactor_;
    cv::Scalar meanValues_;
    bool swapRB_;

    std::vector<std::string> getOutputsNames();

    void preprocess(const cv::Mat& frame, cv::Mat& blob);

    std::vector<cv::Rect> getBoundingBoxes(const cv::Mat& output, const cv::Size& frameSize);
    std::vector<int> getClassIds(const cv::Mat& output);
    std::vector<float> getConfidences(const cv::Mat& output);

    std::vector<int> applyNMS(const std::vector<cv::Rect>& boxes, const std::vector<float>& confidences);
};