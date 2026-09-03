#include "gui.h"

void GuiThreadWorker(CameraGuiData& g_cam1,
                     CameraGuiData& g_cam2,
                     std::atomic<bool>& g_app_running) {

    //  Init GLFW 
    if (!glfwInit()) {
        std::cerr << "[GLFW Error] Failed to initialize GLFW" << std::endl;
        g_app_running = false;
        return;
    }

    // Set OpenGL hints (using OpenGL 3.3 Core Profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create Window & Context
    GLFWwindow* window = glfwCreateWindow(900, 850, "Camera Control Panel", NULL, NULL);
    if (!window) {
        std::cerr << "[GLFW Error] Failed to create window" << std::endl;
        glfwTerminate();
        g_app_running = false;
        return;
    }

    // Crucial: Make the context current on THIS specific worker thread
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable v-sync to limit FPS

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- Initialize GLAD loader ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[GLAD Error] Failed to initialize OpenGL context loader!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        g_app_running = false;
        return;
    }

    // Initialize ImGui contols ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Set up style -> Dark theme 
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

 
    CameraLocalFrames l_cam1;
    CameraLocalFrames l_cam2;

    // Live GUI loop
    while (!glfwWindowShouldClose(window) && g_app_running)
    {
        // Read Windows mouse/keyboard events
        glfwWaitEventsTimeout(0.040); // Wait for an OS input event OR timeout after 33ms (~30 FPS limit)

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // update textures for each camera
         updateCameraTexture(g_cam1, l_cam1, 1);   // Cam 1
        updateCameraTexture(g_cam2, l_cam2, 2);   // Cam 2

        // Rendering commands
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // --- Clean up on exit
    cleanTextrue(l_cam1);
    cleanTextrue(l_cam2);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    g_app_running = false; // Tells other threads to wrap up
    std::cout << "[Thread Exit] GUI Thread stopped gracefully." << std::endl;
}


GLuint MatToTexture(const cv::Mat& mat) {
    if (mat.empty()) return 0;

    // OpenCV uses BGR, OpenGL needs RGB (or RGBA)
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels to the GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgb.cols, rgb.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);

    return textureID;
}


void updateCameraTexture(CameraGuiData& g_cam, CameraLocalFrames& l_cam, int cam_idx) {

    int ui_max_area_limit = g_cam.g_param_max_area.load();   // Max area
    int ui_min_area_limit = g_cam.g_param_min_area.load();   // Min area
    int ui_threshold = g_cam.g_param_threshold.load();       //Theshold

    // Check for new frames from the vision pipeline
    bool update_texture = false;
    {
        std::lock_guard<std::mutex> lock(g_cam.g_frame_mutex);
        if (g_cam.g_new_frame_available) {
            l_cam.ui_local_frame1 = g_cam.g_shared_frame1.clone();
            l_cam.ui_local_frame2 = g_cam.g_shared_frame2.clone();
            l_cam.ui_local_frame3 = g_cam.g_shared_frame3.clone();
            g_cam.g_new_frame_available = false;
            update_texture = true;
        }
    }

    // Texture upload safely executes on this thread
    if (update_texture) {
        if (l_cam.ui_texture_id1 != 0) glDeleteTextures(1, &l_cam.ui_texture_id1);
        if (l_cam.ui_texture_id2 != 0) glDeleteTextures(1, &l_cam.ui_texture_id2);
        if (l_cam.ui_texture_id3 != 0) glDeleteTextures(1, &l_cam.ui_texture_id3);

        if (!l_cam.ui_local_frame1.empty()) l_cam.ui_texture_id1 = MatToTexture(l_cam.ui_local_frame1);
        if (!l_cam.ui_local_frame2.empty()) l_cam.ui_texture_id2 = MatToTexture(l_cam.ui_local_frame2);
        if (!l_cam.ui_local_frame3.empty()) l_cam.ui_texture_id3 = MatToTexture(l_cam.ui_local_frame3);
    }

    std::string cam_index = "Camera " + std::to_string(cam_idx);

    // Adjust  numbers based on your picture dimensions and slider spacing
    ImVec2 fixed_size(850.0f, 400.0f);

    // ImGuiCond_Always forces the layout to respect this size every single time the app runs
    ImGui::SetNextWindowSize(fixed_size, ImGuiCond_Always);

    // This removes the tiny dragging triangle from the bottom-right corner of the window
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize;

    ImGui::Begin(cam_index.c_str(),NULL, window_flags);

    // Parameters inputs
    if (ImGui::SliderInt("Max area limit", &ui_max_area_limit, 100, 20000)) { g_cam.g_param_max_area.store(ui_max_area_limit); }
    if (ImGui::SliderInt("Min area limit", &ui_min_area_limit, 10, 1000)) { g_cam.g_param_min_area.store(ui_min_area_limit); }
    if (ImGui::SliderInt("Threshold", &ui_threshold, 0, 255)) { g_cam.g_param_threshold.store(ui_threshold); }

    ImGui::Separator();
    ImGui::Text("Processing Images:");

    // Image 1
    if (l_cam.ui_texture_id1 != 0) {
        ImGui::BeginGroup(); // Bundles text and image together vertically
        ImGui::Text("Amplitude pic:");
        ImGui::Image((ImTextureID)(intptr_t)l_cam.ui_texture_id1, ImVec2(l_cam.ui_local_frame1.cols, l_cam.ui_local_frame1.rows));
        ImGui::EndGroup();
    }

    // Image 2
    if (l_cam.ui_texture_id2 != 0) {
        ImGui::SameLine(); // Forces next element to draw on the right, instead of underneath
        ImGui::BeginGroup();
        ImGui::Text("Distance pic:");
        ImGui::Image((ImTextureID)(intptr_t)l_cam.ui_texture_id2, ImVec2(l_cam.ui_local_frame2.cols, l_cam.ui_local_frame2.rows));
        ImGui::EndGroup();
    }

    // Image 3
    if (l_cam.ui_texture_id3 != 0) {
        ImGui::SameLine(); // Forces next element to draw on the right, instead of underneath
        ImGui::BeginGroup();
        ImGui::Text("Calculated pic");
        ImGui::Image((ImTextureID)(intptr_t)l_cam.ui_texture_id3, ImVec2(l_cam.ui_local_frame3.cols, l_cam.ui_local_frame3.rows));
        ImGui::EndGroup();
    }


    ImGui::End();
}

void cleanTextrue(CameraLocalFrames& l_cam) {

    if (l_cam.ui_texture_id1 != 0) glDeleteTextures(1, &l_cam.ui_texture_id1);
    if (l_cam.ui_texture_id2 != 0) glDeleteTextures(1, &l_cam.ui_texture_id2);
    if (l_cam.ui_texture_id3 != 0) glDeleteTextures(1, &l_cam.ui_texture_id3);

}