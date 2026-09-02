#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <tuple>
#include <opencv2/opencv.hpp>
#include <ifm3d/device.h>
#include <ifm3d/fg.h>
#include "angle_tracking.h"
#include "camera_data.h"

// Define a static reference vector or axis if needed (Default is the horizontal X-axis: 0 degrees)
const double STATIC_AXIS_ANGLE = 0.0;

// process a frame and calculate relative angle - version for Amplitude frame!
std::tuple<cv::Mat, CameraData> processAndFindRotationAmpl(const cv::Mat& raw_amplitude, int threshold, int min_area);

// process a frame and calculate relative angle - version for Distance frame!
std::tuple<cv::Mat, CameraData> processAndFindRotationDist(const cv::Mat& raw_distance, int threshold, int min_area);

cv::Mat pictureProcessAndGetAmplitude(ifm3d::Buffer& ifm_amplitude);

cv::Mat pictureProcessAndGetDistanceHuman(ifm3d::Buffer& ifm_distance);  // prepare frame for visulazation (colored)
cv::Mat pictureProcessAndGetDistance(ifm3d::Buffer& ifm_distance);       // prepare frame for analyze (gray-scale)