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


std::tuple<cv::Mat, CameraData> processAndFindRotation(const cv::Mat& raw_amplitude, int threshold, int min_area);

cv::Mat pictureProcessAndGetAmplitude(ifm3d::Buffer& ifm_amplitude);
cv::Mat pictureProcessAndGetDistance(ifm3d::Buffer& ifm_distance);

