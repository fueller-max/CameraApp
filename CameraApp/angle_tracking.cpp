
#include "angle_tracking.h"

std::tuple<cv::Mat, CameraData> processAndFindRotationDist(const cv::Mat& distance_masked, 
                                                           uint16_t min_area_limit,
                                                           uint16_t max_area_limit,
                                                           uint16_t errosion_size,
                                                           uint16_t dillitation_size) {
                                                                                

    double relative_angle = 0.0;
   
    //=============================================================================
    //                       Image  Filtering
    //=============================================================================
    
    //morphology: errosion
    if (errosion_size >= 1) {
        cv::Mat kernel_open = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(errosion_size, errosion_size));
        cv::morphologyEx(distance_masked, distance_masked, cv::MORPH_OPEN, kernel_open);                                // open ( imply errosion )
    }
    
    //morphology: dillataion
    if (dillitation_size >= 1) {
        cv::Mat kernel_close = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(dillitation_size, dillitation_size));
        cv::morphologyEx(distance_masked, distance_masked, cv::MORPH_CLOSE, kernel_close);                              // close ( imply dillitation )
    }
    
    //=============================================================================
    //                            Contour Extraction
    //=============================================================================
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(distance_masked, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double max_area = 0;
    int largest_contour_idx = -1;

    for (size_t i = 0; i < contours.size(); i++) {
        
        double area = cv::contourArea(contours[i]);

        if (area > max_area && 
            area > min_area_limit && 
            area < max_area_limit)
        {
            max_area = area;
            largest_contour_idx = static_cast<int>(i);
        }
    }

    //=============================================================================
    //                        Rotation angle calculation
    //============================================================================= 
    if (largest_contour_idx != -1) {
        cv::RotatedRect rotated_box = cv::minAreaRect(contours[largest_contour_idx]);

        // Fix for modern OpenCV versions (4.x+): minAreaRect returns angles between 0 and 90 degrees.
        // It always measures from the horizontal axis down to the first side it meets.
        double detected_angle = rotated_box.angle;

        if (rotated_box.size.width < rotated_box.size.height) {
            detected_angle += 90.0;
        }

        relative_angle = detected_angle - STATIC_AXIS_ANGLE;

        cv::Point2f vertices[4];
        rotated_box.points(vertices);
        for (int i = 0; i < 4; i++) {
            cv::line(distance_masked, vertices[i], vertices[(i + 1) % 4], cv::Scalar(255, 0, 0), 2);
        }

        // - Mark the center of rotation
        cv::circle(distance_masked, rotated_box.center, 5, cv::Scalar(255, 0, 0), -1);

        // - Render the calculated angle onto the frame
        std::string angle_text = "Rotation: " + cv::format("%.2f", relative_angle) + " deg";
        cv::putText(distance_masked, angle_text, cv::Point(10, 20),
            cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 0), 2);

        std::cout << "Object detected at Center [" << rotated_box.center.x << ", " << rotated_box.center.y
            << "] | Angle: " << relative_angle << "°" << std::endl;
    }

    
    CameraData cam_data{ (largest_contour_idx != -1), static_cast<int16_t>(relative_angle) };

    return std::make_tuple(distance_masked, cam_data);
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


cv::Mat pictureProcessAndGetDistanceHuman(ifm3d::Buffer& ifm_distance) {

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

cv::Mat pictureProcessAndGetDistance(ifm3d::Buffer& ifm_distance, uint16_t threshold_mm) {

    int rows = ifm_distance.Height();
    int cols = ifm_distance.Width();

    cv::Mat output_mask(rows, cols, CV_8UC1);
    
    if (ifm_distance.DataFormat() == ifm3d::PixelFormat::FORMAT_16U) {

        //Make masking (thresholding) operation on raw distance pic
        //Keep values less than threshold mm (objects closer than the conveyor belt)
        //Also explicitly filter out 0 (sensor noise / no-return). IFM returns "0" for invalid pixels 
        for (int i = 0; i < rows; ++i) {
 
            const uint16_t* src_ptr = ifm_distance.Ptr<uint16_t>(i);
            uchar* dst_ptr = output_mask.ptr<uchar>(i);

            for (int j = 0; j < cols; ++j) {
                uint16_t val = src_ptr[j];

                if (val != 0 && val < threshold_mm) {
                    dst_ptr[j] = 255; 
                }
                else {
                    dst_ptr[j] = 0;   
                }
            }
        }
     
        return output_mask; 
    
    }
    else
    {
        std::cout << "Wrong pixel format from iFM!";
    }


    // Return the native single-channel 16-bit map directly for processing
    return cv::Mat(ifm_distance.Height(), ifm_distance.Width(), CV_16UC1, ifm_distance.Ptr<uint16_t>(0));
    
}