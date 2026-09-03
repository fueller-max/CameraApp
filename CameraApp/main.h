#pragma once
#include "camera_gui_data.h"

//=========================Shared data & syncronisation for GUI======================================
//   

inline CameraGuiData g_cam1;
inline CameraGuiData g_cam2;

// --- system LCS control ---
inline std::atomic<bool> g_app_running{ true }; // Global running state across all threads

//===================================================================================================

