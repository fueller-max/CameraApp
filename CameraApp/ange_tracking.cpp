
#include "angle_tracking.h"

// Define a static reference vector or axis if needed (Default is the horizontal X-axis: 0 degrees)
const double STATIC_AXIS_ANGLE = 0.0;

std::tuple<cv::Mat, CameraData> processAndFindRotation(const cv::Mat& raw_amplitude, int threshold, int min_area) {

    double relative_angle = 0.0;

    //=============================================================================
    //                       Image Preparation & Filtering
    //=============================================================================
    cv::Mat visual_8u, blurred, threshed;

    //raw -> visulal_8u
    cv::normalize(raw_amplitude, visual_8u, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // Smooth out ToF sensor noise
    //visuall_8u -> blurred
    cv::GaussianBlur(visual_8u, blurred, cv::Size(5, 5), 0);

    // Thresholding to segment the object from the background
    // Adjust the threshold value based on object's reflectivity/distance
    //blurred -> threshed
    cv::threshold(blurred, threshed, threshold, 255, cv::THRESH_BINARY);

    // Convert to color image draw colorful bounding boxes and text
    cv::Mat display_output;
    cv::cvtColor(visual_8u, display_output, cv::COLOR_GRAY2BGR);
    //=============================================================================
     
    //=============================================================================
    //                            Contour Extraction
    //=============================================================================
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(threshed, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
   
  
    // Isolate the largest detected object (ignoring background specks)
    double max_area = 0;
    int largest_contour_idx = -1;

    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);
        if (area > max_area && area > 100) { // Reject contours smaller than 100 pixels
            max_area = area;
            largest_contour_idx = static_cast<int>(i);
        }
    }
    //=============================================================================
   
    //=============================================================================
    //                        Roation angle calculation
    //============================================================================= 
    
    //Calculate Rotation Angle if an object is found
    if (largest_contour_idx != -1) {
        // Fit a minimum area rotated rectangle around the contour
        cv::RotatedRect rotated_box = cv::minAreaRect(contours[largest_contour_idx]);

        // Extract the raw angle from the bounding box
        double detected_angle = rotated_box.angle;

        // Handle OpenCV angle convention mapping (adjusts based on long vs short edge orientation)
         if (rotated_box.size.width < rotated_box.size.height) {
           detected_angle += 90.0;
         }

        // Compute relative rotation angle against your static baseline axis
          relative_angle = detected_angle -  STATIC_AXIS_ANGLE;
    
        // Visualization & UI Overlay
        // - Draw the contours
        cv::drawContours(display_output, contours, largest_contour_idx, cv::Scalar(0, 255, 0), 2);

        // - Draw the vertices of the rotated bounding box
        cv::Point2f vertices[4];
        rotated_box.points(vertices);
        for (int i = 0; i < 4; i++) {
            cv::line(display_output, vertices[i], vertices[(i + 1) % 4], cv::Scalar(255, 0, 0), 2);
        }

        // - Mark the center of rotation
        cv::circle(display_output, rotated_box.center, 5, cv::Scalar(0, 0, 255), -1);

        // - Render the calculated angle onto the frame
        std::string angle_text = "Rotation: " + cv::format("%.2f", relative_angle) + " deg";
        cv::putText(display_output, angle_text, cv::Point(10, 20),
            cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);

        std::cout << "Object detected at Center [" << rotated_box.center.x << ", " << rotated_box.center.y
            << "] | Angle: " << relative_angle << "°" << std::endl;
    }

    //================================================================================
    
    // Upscale the image if neccessary
    cv::Mat enlarged_output;
    // Scale factor
    cv::resize(display_output, enlarged_output, cv::Size(), 1.0, 1.0, cv::INTER_NEAREST);

    return { enlarged_output, { (largest_contour_idx != -1 ), static_cast<int16_t> (relative_angle) } };
}


cv::Mat pictureProcessAndGetAmplitude(ifm3d::Buffer& ifm_amplitude) {

    //Map ifm3d data structures into OpenCV cv::Mat containers
    // O3D303 natively uses 16-bit unsigned integers (CV_16UC1) for ToF pixel maps
    cv::Mat amplitude_mat(ifm_amplitude.Height(), ifm_amplitude.Width(), CV_16UC1, ifm_amplitude.Ptr<uint16_t>(0));
 
    // Data Preparation for Visualization (Convert 16-bit to 8-bit grayscale)
    cv::Mat amplitude_visual;

    // Normalize Amplitude: Maps IR return strength to a readable 0-255 spectrum
    cv::normalize(amplitude_mat, amplitude_visual, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    return amplitude_visual;
}


cv::Mat pictureProcessAndGetDistance(ifm3d::Buffer& ifm_distance) {

    //Map ifm3d data structures into OpenCV cv::Mat containers
    // O3D303 natively uses 16-bit unsigned integers (CV_16UC1) for ToF pixel maps
    cv::Mat distance_mat(ifm_distance.Height(), ifm_distance.Width(), CV_16UC1, ifm_distance.Ptr<uint16_t>(0));

    // Data Preparation for Visualization (Convert 16-bit to 8-bit grayscale)
    cv::Mat distance_visual;

    // Normalize Distance: Maps structural depths to a readable 0-255 spectrum
    cv::normalize(distance_mat, distance_visual, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // Optional: Apply a colormap to the distance map for clear depth visualization
    cv::Mat distance_color;
    cv::applyColorMap(distance_visual, distance_color, cv::COLORMAP_JET);

    return distance_color;

}