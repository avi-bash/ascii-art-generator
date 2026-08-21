#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

const std::string ASCII_CHARS = " .:-=+*#%@";

// Function to convert a single grayscale pixel value to an ASCII character
char pixelToAscii(int grayValue) {
    // Map 0-255 range to the length of our ASCII string
    int index = (grayValue * (ASCII_CHARS.length() - 1)) / 255;
    return ASCII_CHARS[index];
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_video>" << std::endl;
        return -1;
    }

    // Open the video file
    cv::VideoCapture cap(argv[1]);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video file." << std::endl;
        return -1;
    }

    // Target width for the terminal display (adjust based on your terminal size)
    const int TARGET_WIDTH = 150; 
    
    // Terminal characters are taller than they are wide (usually a 1:2 aspect ratio).
    // We adjust the height scale so the ASCII output doesn't look vertically stretched.
    double videoAspectRatio = cap.get(cv::CAP_PROP_FRAME_WIDTH) / cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    int targetHeight = static_cast<int>(TARGET_WIDTH / (videoAspectRatio * 2.0));

    cv::Mat frame, grayFrame, resizedFrame;
    std::string asciiFrame;
    
    // Reserve memory upfront to prevent dynamic reallocation during playback loop
    asciiFrame.reserve((TARGET_WIDTH + 1) * targetHeight);

    // Get the frame rate of the original video to approximate playback speed
    double fps = cap.get(cv::CAP_PROP_FPS);
    int delayMs = (fps > 0) ? static_cast<int>(1000 / fps) : 33;

    while (true) {
        cap >> frame; // Read next frame
        if (frame.empty()) break; // End of video

        // 1. Resize the frame down to terminal resolution
        cv::resize(frame, resizedFrame, cv::Size(TARGET_WIDTH, targetHeight));

        // 2. Convert color frame to grayscale (0-255 light intensity)
        cv::cvtColor(resizedFrame, grayFrame, cv::COLOR_BGR2GRAY);

        // 3. Rebuild the ASCII string matrix
        asciiFrame.clear();
        for (int y = 0; y < grayFrame.rows; ++y) {
            for (int x = 0; x < grayFrame.cols; ++x) {
                // Get the brightness value of the current pixel
                uchar grayValue = grayFrame.at<uchar>(y, x);
                asciiFrame += pixelToAscii(grayValue);
            }
            asciiFrame += '\n'; // Add newline at the end of each row
        }

        // 4. Render to terminal
        // Using ANSI escape code \033[H moves the cursor to the top-left corner
        // instead of clearing the screen, which completely eliminates terminal flickering.
        std::cout << "\033[H" << asciiFrame << std::flush;

        // Maintain playback frame rate
        cv::waitKey(delayMs);
    }

    return 0;
}
