#include "ONNXDetector.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

ONNXDetector::ONNXDetector()
    : confidenceThreshold_(0.5f)
    , nmsThreshold_(0.4f)
    , inputSize_(cv::Size(640, 640))
    , scaleFactor_(1.0 / 255.0)
    , meanValues_(cv::Scalar(0, 0, 0))
    , swapRB_(true)
{
}

ONNXDetector::~ONNXDetector()
{
}

bool ONNXDetector::loadModel(const std::string& modelPath, const std::string& classesPath)
{
    try {
        net_ = cv::dnn::readNetFromONNX(modelPath);

        if (net_.empty()) {
            std::cerr << "Failed to load ONNX model from: " << modelPath << std::endl;
            return false;
        }

        // Try to set backend and target
        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

        // Load class names if provided
        if (!classesPath.empty()) {
            std::ifstream classFile(classesPath);
            if (classFile.is_open()) {
                std::string className;
                while (std::getline(classFile, className)) {
                    if (!className.empty()) {
                        classNames_.push_back(className);
                    }
                }
                classFile.close();
                std::cout << "Loaded " << classNames_.size() << " class names" << std::endl;
            } else {
                std::cerr << "Warning: Could not open class names file: " << classesPath << std::endl;
            }
        }

        // If no class names loaded, create default ones
        if (classNames_.empty()) {
            // Create default class names for safety signs
            classNames_ = {
                "caution_shock",
                "caution_oxidizer",
                "caution_radiation"
            };
            std::cout << "Using default class names for safety signs" << std::endl;
        }

        std::cout << "ONNX model loaded successfully: " << modelPath << std::endl;
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV exception while loading model: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Exception while loading model: " << e.what() << std::endl;
        return false;
    }
}

void ONNXDetector::setConfidenceThreshold(float threshold)
{
    confidenceThreshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

void ONNXDetector::setNMSThreshold(float threshold)
{
    nmsThreshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

std::vector<std::string> ONNXDetector::getClassNames() const
{
    return classNames_;
}

void ONNXDetector::preprocess(const cv::Mat& frame, cv::Mat& blob)
{
    cv::dnn::blobFromImage(frame, blob, scaleFactor_, inputSize_, meanValues_, swapRB_, false);
}

std::vector<std::string> ONNXDetector::getOutputsNames()
{
    static std::vector<std::string> names;
    if (names.empty()) {
        std::vector<int> outLayers = net_.getUnconnectedOutLayers();
        std::vector<std::string> layersNames = net_.getLayerNames();
        names.resize(outLayers.size());
        for (size_t i = 0; i < outLayers.size(); ++i) {
            names[i] = layersNames[outLayers[i] - 1];
        }
    }
    return names;
}

std::vector<cv::Rect> ONNXDetector::getBoundingBoxes(const cv::Mat& output, const cv::Size& frameSize)
{
    std::vector<cv::Rect> boxes;

    // Assuming output is in YOLO format: [batch, num_detections, 85]
    // where 85 = [x, y, w, h, confidence, class_probabilities...]

    const int numDetections = output.size[1];
    const int numClasses = output.size[2] - 5; // 5 for x,y,w,h,confidence

    for (int i = 0; i < numDetections; ++i) {
        float confidence = output.at<float>(0, i, 4);

        if (confidence > confidenceThreshold_) {
            // Find class with maximum probability
            int classId = 0;
            float maxClassProb = 0.0f;
            for (int j = 0; j < numClasses; ++j) {
                float classProb = output.at<float>(0, i, 5 + j);
                if (classProb > maxClassProb) {
                    maxClassProb = classProb;
                    classId = j;
                }
            }

            float finalConfidence = confidence * maxClassProb;
            if (finalConfidence > confidenceThreshold_) {
                // Get bounding box coordinates (normalized to 0-1)
                float centerX = output.at<float>(0, i, 0);
                float centerY = output.at<float>(0, i, 1);
                float width = output.at<float>(0, i, 2);
                float height = output.at<float>(0, i, 3);

                // Convert to pixel coordinates
                int left = static_cast<int>((centerX - width / 2) * frameSize.width);
                int top = static_cast<int>((centerY - height / 2) * frameSize.height);
                int right = static_cast<int>((centerX + width / 2) * frameSize.width);
                int bottom = static_cast<int>((centerY + height / 2) * frameSize.height);

                // Ensure coordinates are within image bounds
                left = std::max(0, left);
                top = std::max(0, top);
                right = std::min(frameSize.width - 1, right);
                bottom = std::min(frameSize.height - 1, bottom);

                boxes.push_back(cv::Rect(left, top, right - left, bottom - top));
            }
        }
    }

    return boxes;
}

std::vector<int> ONNXDetector::getClassIds(const cv::Mat& output)
{
    std::vector<int> classIds;

    const int numDetections = output.size[1];
    const int numClasses = output.size[2] - 5;

    for (int i = 0; i < numDetections; ++i) {
        float confidence = output.at<float>(0, i, 4);

        if (confidence > confidenceThreshold_) {
            int classId = 0;
            float maxClassProb = 0.0f;
            for (int j = 0; j < numClasses; ++j) {
                float classProb = output.at<float>(0, i, 5 + j);
                if (classProb > maxClassProb) {
                    maxClassProb = classProb;
                    classId = j;
                }
            }

            float finalConfidence = confidence * maxClassProb;
            if (finalConfidence > confidenceThreshold_) {
                classIds.push_back(classId);
            }
        }
    }

    return classIds;
}

std::vector<float> ONNXDetector::getConfidences(const cv::Mat& output)
{
    std::vector<float> confidences;

    const int numDetections = output.size[1];
    const int numClasses = output.size[2] - 5;

    for (int i = 0; i < numDetections; ++i) {
        float confidence = output.at<float>(0, i, 4);

        if (confidence > confidenceThreshold_) {
            float maxClassProb = 0.0f;
            for (int j = 0; j < numClasses; ++j) {
                float classProb = output.at<float>(0, i, 5 + j);
                if (classProb > maxClassProb) {
                    maxClassProb = classProb;
                }
            }

            float finalConfidence = confidence * maxClassProb;
            if (finalConfidence > confidenceThreshold_) {
                confidences.push_back(finalConfidence);
            }
        }
    }

    return confidences;
}

std::vector<int> ONNXDetector::applyNMS(const std::vector<cv::Rect>& boxes, const std::vector<float>& confidences)
{
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, confidenceThreshold_, nmsThreshold_, indices);
    return indices;
}

std::vector<DetectionResult> ONNXDetector::detect(const cv::Mat& image)
{
    std::vector<DetectionResult> detections;

    if (net_.empty() || image.empty()) {
        return detections;
    }

    try {
        // Preprocess image
        cv::Mat blob;
        preprocess(image, blob);

        // Set input
        net_.setInput(blob);

        // Forward pass
        std::vector<cv::Mat> outputs;
        net_.forward(outputs, getOutputsNames());

        if (outputs.empty()) {
            return detections;
        }

        // Get detections from the first output
        cv::Mat& output = outputs[0];

        // Get bounding boxes, class IDs and confidences
        std::vector<cv::Rect> boxes = getBoundingBoxes(output, image.size());
        std::vector<int> classIds = getClassIds(output);
        std::vector<float> confidences = getConfidences(output);

        // Apply Non-Maximum Suppression
        std::vector<int> indices = applyNMS(boxes, confidences);

        // Collect final detections with bounding boxes
        for (size_t i = 0; i < indices.size(); ++i) {
            int idx = indices[i];
            if (idx < classIds.size() && idx < confidences.size() && idx < boxes.size()) {
                int classId = classIds[idx];
                float confidence = confidences[idx];
                cv::Rect box = boxes[idx];

                if (classId < classNames_.size()) {
                    detections.emplace_back(classNames_[classId], confidence, box);
                }
            }
        }

    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV exception during detection: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception during detection: " << e.what() << std::endl;
    }

    return detections;
}

void ONNXDetector::drawDetections(cv::Mat& image, const std::vector<DetectionResult>& detections)
{
    if (detections.empty()) {
        return;
    }

    // Define colors for different classes
    std::map<std::string, cv::Scalar> classColors = {
        {"caution_shock", cv::Scalar(0, 0, 255)},      // BGR: Red
        {"caution_oxidizer", cv::Scalar(0, 255, 255)}, // BGR: Yellow
        {"caution_radiation", cv::Scalar(255, 0, 0)},  // BGR: Blue
        {"first_mark", cv::Scalar(0, 255, 0)},  // BGR: Green
        {"second_mark", cv::Scalar(255, 0, 255)}, // BGR: Magenta
    };

    // Draw each detection
    for (size_t i = 0; i < detections.size(); ++i) {
        const auto& detection = detections[i];
        const std::string& className = detection.className;
        float confidence = detection.confidence;
        const cv::Rect& box = detection.boundingBox;

        // Get color for this class
        cv::Scalar color = classColors[className];
        if (color == cv::Scalar(0, 0, 0)) {
            color = cv::Scalar(255, 255, 255); // Default to white
        }

        // Create label
        std::string label = className + ": " + std::to_string(static_cast<int>(confidence * 100)) + "%";

        // Draw bounding box
        cv::rectangle(image, box, color, 2);

        // Draw filled rectangle for label background
        int baseline = 0;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

        cv::rectangle(image,
                     cv::Point(box.x, box.y - labelSize.height - 5),
                     cv::Point(box.x + labelSize.width, box.y),
                     color, cv::FILLED);

        // Draw label above bounding box
        cv::putText(image, label,
                   cv::Point(box.x, box.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1); // Black text

        // Also draw label at top of image for easy viewing
        int yPos = 25 * (i + 1);
        cv::putText(image, label, cv::Point(10, yPos),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
    }

    // Draw a thin border around the entire image when detections are present
    if (!detections.empty()) {
        cv::rectangle(image, cv::Point(0, 0),
                     cv::Point(image.cols - 1, image.rows - 1),
                     cv::Scalar(200, 200, 200), 1); // Light gray border
    }
}