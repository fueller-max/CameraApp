#include "gui.h"

void GuiThreadWorker(std::mutex& g_frame_mutex,
                     cv::Mat& g_shared_output_frame, 
                     bool& g_new_frame_available, 
                     std::atomic<float>& g_param_angle,
                     std::atomic<int>& g_param_threshold,
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
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Control Panel", NULL, NULL);
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

    cv::Mat ui_local_frame;  //local frame for the picture
    GLuint ui_texture_id = 0; // The actual OpenGL texture handle

    //Process parameters:
    float ui_angle = g_param_angle.load();       // Angle
    int ui_threshold = g_param_threshold.load(); //Theshold

    // Live GUI loop
    while (!glfwWindowShouldClose(window) && g_app_running)
    {
        // Read Windows mouse/keyboard events
        glfwWaitEventsTimeout(0.033); // Wait for an OS input event OR timeout after 33ms (~30 FPS limit)

        // Check for new frames from the vision pipeline
        bool update_texture = false;
        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            if (g_new_frame_available) {
                ui_local_frame = g_shared_output_frame.clone();
                g_new_frame_available = false;
                update_texture = true;
            }
        }

        // Texture upload safely executes on this thread
        if (update_texture && !ui_local_frame.empty()) {
            if (ui_texture_id != 0) glDeleteTextures(1, &ui_texture_id);
            ui_texture_id = MatToTexture(ui_local_frame); // Call conversion cv:Mat -> Texture
        }

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Object Tracking & Angle Detection");

        if (ImGui::SliderFloat("Target Angle", &ui_angle, 0.0f, 360.0f)) {
            g_param_angle.store(ui_angle);
        }
        if (ImGui::SliderInt("Threshold", &ui_threshold, 0, 255)) {
            g_param_threshold.store(ui_threshold);
        }

        ImGui::Separator();

        // Draw the image if it is loaded
        if (ui_texture_id != 0) {
            ImGui::Image((void*)(intptr_t)ui_texture_id, ImVec2((float)ui_local_frame.cols, (float)ui_local_frame.rows));
        }

        ImGui::End();

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
    if (ui_texture_id != 0) glDeleteTextures(1, &ui_texture_id);

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