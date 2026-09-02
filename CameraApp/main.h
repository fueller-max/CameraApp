#pragma once

//=========================Shared data & syncronisation for GUI======================================
//   

inline std::mutex g_frame_mutex;                                     // Mutex for sync data
inline cv::Mat g_shared_frame1, g_shared_frame2, g_shared_frame3;   // Frames shared between threads
inline bool g_new_frame_available = false;                          // Flag to tell UI thread a new frame is ready

// ---  Atomic parameters (GUI Thread <-> OpenCV Thread) 
inline std::atomic<int> g_param_min_area{ 100 };
inline std::atomic<int> g_param_threshold{ 100 };

// --- system LCS control ---
inline std::atomic<bool> g_app_running{ true }; // Global running state across all threads

//===================================================================================================

