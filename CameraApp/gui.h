#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <opencv2/opencv.hpp>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <mutex>



void GuiThreadWorker(std::mutex& g_frame_mutex, 
	                 cv::Mat& g_shared_output_frame,
	                 bool& g_new_frame_available,
	                 std::atomic<float>& g_param_angle,
	                 std::atomic<int>& g_param_threshold,
	                 std::atomic<bool>& g_app_running);

// Function to convert cv::Mat to OpenGL Texture
GLuint MatToTexture(const cv::Mat& mat); 