#include "gui.h"
#include "saved_settings.h"

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
    GLFWwindow* window = glfwCreateWindow(900, 800, "Camera Control Panel", NULL, NULL);
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

        if (saved_settings::g_settings_dirty) {
            // Accumulate the time passed since the last frame
        saved_settings::g_save_timer += ImGui::GetIO().DeltaTime;

            // If the user hasn't touched anything for 1.5 seconds, commit to disk
            if (saved_settings::g_save_timer >= saved_settings::SAVE_DELAY_THRESHOLD) {
                saved_settings::SavedSettings snapshot_cam1{};
                snapshot_cam1.param_max_area = g_cam1.g_param_max_area.load();
                snapshot_cam1.param_min_area = g_cam1.g_param_min_area.load();
				snapshot_cam1.param_errosion_size = g_cam1.g_param_errosion_size.load();
				snapshot_cam1.param_dillitation_size = g_cam1.g_param_dillitation_size.load();
				snapshot_cam1.param_distance_mm = g_cam1.g_param_distance_mm.load();

                saved_settings::SavedSettings snapshot_cam2{};
                snapshot_cam2.param_max_area = g_cam2.g_param_max_area.load();
                snapshot_cam2.param_min_area = g_cam2.g_param_min_area.load();
                snapshot_cam2.param_errosion_size = g_cam2.g_param_errosion_size.load();
                snapshot_cam2.param_dillitation_size = g_cam2.g_param_dillitation_size.load();
                snapshot_cam2.param_distance_mm = g_cam2.g_param_distance_mm.load();


                saved_settings::DualCameraSettings masterSnapshot;
                masterSnapshot.cam1 = snapshot_cam1;
                masterSnapshot.cam2 = snapshot_cam2;

                // Binary writing logic -> write settings on disk
                std::ofstream outFile(saved_settings::filename, std::ios::binary);
                if (outFile) {
                    outFile.write(reinterpret_cast<const char*>(&masterSnapshot), sizeof(saved_settings::DualCameraSettings));
                }

                // Reset dirty state
                saved_settings::g_settings_dirty = false;
                saved_settings::g_save_timer = 0.0f;
            }
        }
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
    std::cout << "GUI Thread stopped" << std::endl;
}


GLuint MatToTexture(const cv::Mat& mat, GLuint existingTextureID) {
    if (mat.empty()) return 0;

    GLenum inputFormat = GL_RGB;
    cv::Mat tempMat;

    // Handle data format safely based on image channels
    if (mat.channels() == 4) {
        cv::cvtColor(mat, tempMat, cv::COLOR_BGRA2RGBA);
        inputFormat = GL_RGBA;
    }
    else if (mat.channels() == 3) {
        cv::cvtColor(mat, tempMat, cv::COLOR_BGR2RGB);
        inputFormat = GL_RGB;
    }
    else if (mat.channels() == 1) {
        tempMat = mat;
        inputFormat = GL_RED;
    }

    GLuint textureID = existingTextureID;

    // If no texture exists yet, generate it once
    if (textureID == 0) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Single-channel setup fix: Map the Single Red Channel pixel value to Red, Green, and Blue
        if (mat.channels() == 1) {
            GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        }

        // Allocate the data store on the GPU
        glTexImage2D(GL_TEXTURE_2D, 0, (mat.channels() == 1) ? GL_RGB : inputFormat,
            tempMat.cols, tempMat.rows, 0, inputFormat, GL_UNSIGNED_BYTE, tempMat.data);
    }
    else {
        // Texture already exists on the GPU! Just overwrite the pixels directly 
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tempMat.cols, tempMat.rows, inputFormat, GL_UNSIGNED_BYTE, tempMat.data);
    }

    return textureID;
}


void updateCameraTexture(CameraGuiData& g_cam, CameraLocalFrames& l_cam, int cam_idx) {

    int ui_max_area_limit = g_cam.g_param_max_area.load();              // Max area
    int ui_min_area_limit = g_cam.g_param_min_area.load();              // Min area
    int ui_errosion_size = g_cam.g_param_errosion_size.load();          // Errosion  size
    int ui_dillitation_size = g_cam.g_param_dillitation_size.load();    // Dillitation size
    int g_param_distance_mm = g_cam.g_param_distance_mm.load();         // Distance theshold

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
        if (!l_cam.ui_local_frame1.empty()) {
            l_cam.ui_texture_id1 = MatToTexture(l_cam.ui_local_frame1, l_cam.ui_texture_id1);
        }
        if (!l_cam.ui_local_frame2.empty()) {
            l_cam.ui_texture_id2 = MatToTexture(l_cam.ui_local_frame2, l_cam.ui_texture_id2);
        }
        if (!l_cam.ui_local_frame3.empty()) {
            l_cam.ui_texture_id3 = MatToTexture(l_cam.ui_local_frame3, l_cam.ui_texture_id3);
        }
    }

    std::string cam_index = "Camera " + std::to_string(cam_idx);
    ImVec2 fixed_size(850.0f, 400.0f); //numbers based on picture dimensions and slider spacing
    ImGui::SetNextWindowSize(fixed_size, ImGuiCond_Always);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize;

    ImGui::Begin(cam_index.c_str(),NULL, window_flags);

    // --- CAMERA CONNECTION STATUS INDICATOR ---
    float circle_radius = 6.0f;

    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(circle_radius * 2.0f + 4.0f, circle_radius * 2.0f));

    // Calculate the absolute center position for drawing the circle
    ImVec2 circle_center = ImVec2(cursor_pos.x + circle_radius, cursor_pos.y + circle_radius + 2.0f);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Select color and text based on the connection state variable we created in step 1
    ImU32 status_color;
    std::string status_text;

    if (g_cam.is_connected) {
        status_color = IM_COL32(0, 255, 0, 255);       // Green
        status_text = "Camera connected";
    }
    else {
        status_color = IM_COL32(255, 0, 0, 255);       // Red
        status_text = "Camera not connected";
    }

    // Draw the circle and append the text to its right side
    draw_list->AddCircleFilled(circle_center, circle_radius, status_color);
    ImGui::SameLine();
    ImGui::Text("%s", status_text.c_str());

    ImGui::Separator();
    // ==========================================


    // Parameters inputs
    if (ImGui::SliderInt("Max area limit", &ui_max_area_limit, 100, 20000)) { 
        g_cam.g_param_max_area.store(ui_max_area_limit);
        saved_settings::make_settings_dirty();
    }
    if (ImGui::SliderInt("Min area limit", &ui_min_area_limit, 10, 1000)) { 
        g_cam.g_param_min_area.store(ui_min_area_limit);
        saved_settings::make_settings_dirty();
    }
    if (ImGui::SliderInt("Distance threshold, mm", &g_param_distance_mm, 1000, 1500)) {
        g_cam.g_param_distance_mm.store(g_param_distance_mm);
        saved_settings::make_settings_dirty();
    }
    if (ImGui::SliderInt("Errosion size", &ui_errosion_size, 0, 10)) { 
        g_cam.g_param_errosion_size.store(ui_errosion_size);
        saved_settings::make_settings_dirty();
    }
    if (ImGui::SliderInt("Dilitation size", &ui_dillitation_size, 0, 10)) { 
        g_cam.g_param_dillitation_size.store(ui_dillitation_size);
        saved_settings::make_settings_dirty();
    }
    

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

    if (l_cam.ui_texture_id1 != 0) { glDeleteTextures(1, &l_cam.ui_texture_id1); l_cam.ui_texture_id1 = 0; }
    if (l_cam.ui_texture_id2 != 0) { glDeleteTextures(1, &l_cam.ui_texture_id2); l_cam.ui_texture_id2 = 0; }
    if (l_cam.ui_texture_id3 != 0) { glDeleteTextures(1, &l_cam.ui_texture_id3); l_cam.ui_texture_id3 = 0; }
}