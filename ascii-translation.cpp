#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <stdexcept>
#include <vector>
#include <opencv2/opencv.hpp>

const std::string ASCII_CHARS = " .:-=+*#%@";

// Function to convert a single grayscale pixel value to an ASCII character
int main(int argc, char** argv) {
    bool enableGlow = false;
    for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex) {
        if (std::string(argv[argumentIndex]) == "--glow") {
            enableGlow = true;
            for (int shiftIndex = argumentIndex; shiftIndex + 1 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 1];
            }
            --argc;
            --argumentIndex;
        }
    }
    const std::string videoPath = argc >= 2 ? argv[1] : "bird.mp4";
    cv::Scalar textColor(255, 80, 255);

    if (argc == 5) {
        try {
            const int red = std::stoi(argv[2]);
            const int green = std::stoi(argv[3]);
            const int blue = std::stoi(argv[4]);
            if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255) {
                throw std::out_of_range("color component");
            }
            textColor = cv::Scalar(blue, green, red);
        } catch (const std::exception&) {
            std::cerr << "Color must be three integers from 0 to 255: red green blue" << std::endl;
            return -1;
        }
    } else if (argc != 1 && argc != 2) {
        std::cerr << "Usage: ascii-translation.exe [video-path] [red] [green] [blue] [--glow]" << std::endl;
        return -1;
    }

    // Open the video file
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video file: " << videoPath << std::endl;
        std::cerr << "Pass a video path as the first argument, or place bird.mp4 in the project folder." << std::endl;
        return -1;
    }

    // Target width for the window display (adjust based on your screen size)
    const int TARGET_WIDTH = 150;
    
    // Terminal characters are taller than they are wide (usually a 1:2 aspect ratio).
    // We adjust the height scale so the ASCII output doesn't look vertically stretched.
    const int VIDEO_WIDTH = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    const int VIDEO_HEIGHT = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    if (VIDEO_WIDTH <= 0 || VIDEO_HEIGHT <= 0) {
        std::cerr << "Error: Video dimensions could not be read." << std::endl;
        return -1;
    }
    double videoAspectRatio = static_cast<double>(VIDEO_WIDTH) / VIDEO_HEIGHT;
    const int targetHeight = std::max(1, static_cast<int>(TARGET_WIDTH / (videoAspectRatio * 2.0)));

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

    std::vector<cv::Mat> glyphs;
    std::vector<cv::Mat> glyphMasks;
    glyphs.reserve(ASCII_CHARS.length());
    glyphMasks.reserve(ASCII_CHARS.length());
    for (const char character : ASCII_CHARS) {
        cv::Mat glyph(LINE_HEIGHT, CHAR_WIDTH, CV_8UC3, cv::Scalar(0, 0, 0));
        cv::Mat glyphMask(LINE_HEIGHT, CHAR_WIDTH, CV_8UC1, cv::Scalar(0));
        cv::putText(glyph, std::string(1, character), cv::Point(0, LINE_HEIGHT - BASELINE),
            FONT_FACE, FONT_SCALE, textColor, FONT_THICKNESS, cv::LINE_AA);
        cv::putText(glyphMask, std::string(1, character), cv::Point(0, LINE_HEIGHT - BASELINE),
            FONT_FACE, FONT_SCALE, cv::Scalar(255), FONT_THICKNESS, cv::LINE_AA);
        glyphs.push_back(glyph);
        glyphMasks.push_back(glyphMask);
    }

    cv::namedWindow("ASCII Video", cv::WINDOW_NORMAL);
    cv::resizeWindow("ASCII Video", WINDOW_WIDTH, WINDOW_HEIGHT);

    // Get the frame rate of the original video to approximate playback speed
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (!(fps > 0.0) || !std::isfinite(fps)) fps = 30.0;
    const auto framePeriod = std::chrono::duration<double>(1.0 / fps);
    auto nextFrameDeadline = std::chrono::steady_clock::now();

    while (true) {
        while (std::chrono::steady_clock::now() > nextFrameDeadline +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod)) {
            if (!cap.grab()) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if (!cap.grab()) break;
            }
            nextFrameDeadline += std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod);
        }
        cap >> frame; // Read next frame
        if (frame.empty()) {
            if (!cap.set(cv::CAP_PROP_POS_FRAMES, 0)) break;
            cap >> frame;
            if (frame.empty()) break;
        }

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
                const int glyphIndex = grayFrame.ptr<uchar>(y)[x] *
                    (static_cast<int>(ASCII_CHARS.length()) - 1) / 255;
                cv::Mat glyphRegion = asciiImage(
                    cv::Rect(x * CHAR_WIDTH, y * LINE_HEIGHT, CHAR_WIDTH, LINE_HEIGHT));
                glyphs[glyphIndex].copyTo(glyphRegion, glyphMasks[glyphIndex]);
            }
        }

        if (enableGlow) {
            const cv::Size glowSize(
                std::max(1, asciiImage.cols / 4),
                std::max(1, asciiImage.rows / 4));
            cv::Mat glowImage;
            cv::resize(asciiImage, glowImage, glowSize, 0.0, 0.0, cv::INTER_AREA);
            cv::GaussianBlur(glowImage, glowImage, cv::Size(0, 0), 15.0);
            cv::resize(glowImage, glowImage, asciiImage.size(), 0.0, 0.0, cv::INTER_LINEAR);
            cv::addWeighted(asciiImage, 1.0, glowImage, 5, 0.0, asciiImage);
        }

        // 4. Render to the standalone OpenCV window
        cv::imshow("ASCII Video", asciiImage);

        // Maintain playback frame rate and allow the user to quit with Esc or Q
        nextFrameDeadline += std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod);
        const auto now = std::chrono::steady_clock::now();
        if (now > nextFrameDeadline) nextFrameDeadline = now;
        const auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextFrameDeadline - now);
        int key = cv::waitKey(std::max(1, static_cast<int>(waitTime.count())));
        if (key == 27 || key == 'q' || key == 'Q') break;
    }

    cv::destroyAllWindows();
    return 0;
}
