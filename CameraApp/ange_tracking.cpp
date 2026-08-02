
#include "angle_tracking.hpp"

// Define a static reference vector or axis if needed (Default is the horizontal X-axis: 0 degrees)
const double STATIC_AXIS_ANGLE = 0.0;

void processAndFindRotation(const cv::Mat& raw_amplitude) {
    // 1. Image Preparation & Filtering
    cv::Mat visual_8u, blurred, threshed;
    cv::normalize(raw_amplitude, visual_8u, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // Smooth out ToF sensor noise
    cv::GaussianBlur(visual_8u, blurred, cv::Size(5, 5), 0);

    // 2. Thresholding to segment the object from the background
    // Adjust the '100' threshold value based on your object's reflectivity/distance
    cv::threshold(blurred, threshed, 100, 255, cv::THRESH_BINARY);

    // 3. Contour Extraction
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(threshed, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Convert to color image draw colorful bounding boxes and text
    cv::Mat display_output;
    cv::cvtColor(visual_8u, display_output, cv::COLOR_GRAY2BGR);

    // 4. Isolate the largest detected object (ignoring background specks)
    double max_area = 0;
    int largest_contour_idx = -1;

    for (size_t i = 0; i < contours.size(); i++) {
        double area = cv::contourArea(contours[i]);
        if (area > max_area && area > 100) { // Reject contours smaller than 100 pixels
            max_area = area;
            largest_contour_idx = static_cast<int>(i);
        }
    }

    // 5. Calculate Rotation Angle if an object is found
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
         double relative_angle = detected_angle -  STATIC_AXIS_ANGLE;

        // 6. Visualization & UI Overlay
        // Draw the contours
        cv::drawContours(display_output, contours, largest_contour_idx, cv::Scalar(0, 255, 0), 2);

        // Draw the vertices of the rotated bounding box
        cv::Point2f vertices[4];
        rotated_box.points(vertices);
        for (int i = 0; i < 4; i++) {
            cv::line(display_output, vertices[i], vertices[(i + 1) % 4], cv::Scalar(255, 0, 0), 2);
        }

        // Mark the center of rotation
        cv::circle(display_output, rotated_box.center, 5, cv::Scalar(0, 0, 255), -1);

        // Render the calculated angle onto the frame
        std::string angle_text = "Rotation: " + cv::format("%.2f", relative_angle) + " deg";
        cv::putText(display_output, angle_text, cv::Point(20, 40),
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

        std::cout << "Object detected at Center [" << rotated_box.center.x << ", " << rotated_box.center.y
            << "] | Angle: " << relative_angle << "°" << std::endl;
    }

    // Upscale the image so it is easy to view (e.g., scale up by 2x or 3x)
    cv::Mat enlarged_output;
    // Scale 352x264 up to 1056x792 for visibility
    cv::resize(display_output, enlarged_output, cv::Size(), 3.0, 3.0, cv::INTER_NEAREST);

    // Open an explicit named window that allows resizing if needed
    cv::namedWindow("Object Tracking & Angle Detection", cv::WINDOW_AUTOSIZE);

    // Push the frame out
    cv::imshow("Object Tracking & Angle Detection", enlarged_output);

    // Display the live localized object tracking feed
    //cv::imshow("Object Tracking & Angle Detection", display_output);
     cv::waitKey(0);
}
