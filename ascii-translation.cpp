#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>

const std::string ASCII_CHARS = " .:-=+*#%@";
const std::string VIDEO_PATH = "C:\\Users\\avi\\Downloads\\bird.mp4";

// Function to convert a single grayscale pixel value to an ASCII character
char pixelToAscii(int grayValue) {
    // Map 0-255 range to the length of our ASCII string
    int index = (grayValue * (ASCII_CHARS.length() - 1)) / 255;
    return ASCII_CHARS[index];
}

int main(int argc, char** argv) {
    const std::string videoPath = argc >= 2 ? argv[1] : VIDEO_PATH;

    // Open the video file
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video file: " << videoPath << std::endl;
        std::cerr << "Set VIDEO_PATH near the top of the file or pass a path as an argument." << std::endl;
        return -1;
    }

    // Target width for the window display (adjust based on your screen size)
    const int TARGET_WIDTH = 150;
    
    // Terminal characters are taller than they are wide (usually a 1:2 aspect ratio).
    // We adjust the height scale so the ASCII output doesn't look vertically stretched.
    const int VIDEO_WIDTH = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    const int VIDEO_HEIGHT = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double videoAspectRatio = static_cast<double>(VIDEO_WIDTH) / VIDEO_HEIGHT;
    int targetHeight = static_cast<int>(TARGET_WIDTH / (videoAspectRatio * 2.0));

    cv::Mat frame, grayFrame, resizedFrame;

    const int FONT_FACE = cv::FONT_HERSHEY_PLAIN;
    const double FONT_SCALE = 1.0;
    const int FONT_THICKNESS = 1;
    const int CHAR_WIDTH = 10;
    const int LINE_HEIGHT = 14;
    const int BASELINE = 3;
    const int WINDOW_HEIGHT = 780;
    const int WINDOW_WIDTH = static_cast<int>(
        WINDOW_HEIGHT * static_cast<double>(VIDEO_WIDTH) / VIDEO_HEIGHT);
    cv::namedWindow("ASCII Video", cv::WINDOW_NORMAL);
    cv::resizeWindow("ASCII Video", WINDOW_WIDTH, WINDOW_HEIGHT);

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

        // 3. Draw the ASCII frame into an image for the OpenCV window
        cv::Mat asciiImage(
            targetHeight * LINE_HEIGHT + BASELINE,
            TARGET_WIDTH * CHAR_WIDTH,
            CV_8UC3,
            cv::Scalar(0, 0, 0));

        for (int y = 0; y < grayFrame.rows; ++y) {
            for (int x = 0; x < grayFrame.cols; ++x) {
                uchar grayValue = grayFrame.at<uchar>(y, x);
                std::string character(1, pixelToAscii(grayValue));
                cv::putText(
                    asciiImage,
                    character,
                    cv::Point(x * CHAR_WIDTH, (y + 1) * LINE_HEIGHT - BASELINE),
                    FONT_FACE,
                    FONT_SCALE,
                    cv::Scalar(255, 255, 255),
                    FONT_THICKNESS,
                    cv::LINE_AA);
            }
        }

        // 4. Render to the standalone OpenCV window
        cv::imshow("ASCII Video", asciiImage);

        // Maintain playback frame rate and allow the user to quit with Esc or Q
        int key = cv::waitKey(delayMs);
        if (key == 27 || key == 'q' || key == 'Q') break;
    }

    cv::destroyAllWindows();
    return 0;
}
