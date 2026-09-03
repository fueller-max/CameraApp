#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "camera_gui_data.h"
#include <opencv2/opencv.hpp>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <mutex>



void GuiThreadWorker(CameraGuiData& g_cam1,
	                 CameraGuiData& g_cam2,
	                 std::atomic<bool>& g_app_running);

// Function to convert cv::Mat to OpenGL Texture
GLuint MatToTexture(const cv::Mat& mat); 


struct CameraLocalFrames{
	cv::Mat ui_local_frame1, ui_local_frame2, ui_local_frame3;          //local frames for the pictures
	GLuint ui_texture_id1 = 0, ui_texture_id2 = 0, ui_texture_id3 = 0;  // The actual OpenGL texture handles

};

void updateCameraTexture(CameraGuiData& g_cam, CameraLocalFrames& l_cam, int cam_idx);
void cleanTextrue(CameraLocalFrames& l_cam);