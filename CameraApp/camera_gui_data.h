#pragma once

#include <mutex>
#include <atomic>
#include <opencv2/opencv.hpp>

struct CameraGuiData {
    // Mutex for syncing frame data
    std::mutex g_frame_mutex;

    // Frames shared between threads
    cv::Mat g_shared_frame1;
    cv::Mat g_shared_frame2;
    cv::Mat g_shared_frame3;

    // Flag to tell UI thread a new frame is ready
    bool g_new_frame_available = false;

    // Atomic parameters (GUI Thread <-> OpenCV Thread) 
    std::atomic<int> g_param_max_area{ 10000 };
    std::atomic<int> g_param_min_area{ 200 };
    std::atomic<int> g_param_threshold{ 100 };
};