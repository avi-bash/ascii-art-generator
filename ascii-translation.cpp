#include <iostream>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <string>
#include <stdexcept>
#include <vector>
#include <opencv2/opencv.hpp>
#include "settings_window.h"
#include "transparent_window.h"

const std::string ASCII_CHARS = ".:-=+*#%@";
const int TRANSPARENT_BRIGHTNESS_THRESHOLD = 16;

// Function to convert a single grayscale pixel value to an ASCII character
int main(int argc, char** argv) {
    bool enableGlow = false;
    bool transparent = true;
    int transparentBrightnessThreshold = TRANSPARENT_BRIGHTNESS_THRESHOLD;
    double glowStrength = 5.0;
    double glowFalloff = 5.0;
    double glowResolution = 0.5;
    int glowRadius = 0;
    for (int argumentIndex = 1; argumentIndex < argc; ++argumentIndex) {
        if (std::string(argv[argumentIndex]) == "--glow" ||
            std::string(argv[argumentIndex]) == "-g") {
            enableGlow = true;
            for (int shiftIndex = argumentIndex; shiftIndex + 1 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 1];
            }
            --argc;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--transparent" ||
               std::string(argv[argumentIndex]) == "-t") {
            transparent = true;
            for (int shiftIndex = argumentIndex; shiftIndex + 1 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 1];
            }
            --argc;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--opaque" ||
               std::string(argv[argumentIndex]) == "-o" ||
                   std::string(argv[argumentIndex]) == "--no-transparent") {
            transparent = false;
            for (int shiftIndex = argumentIndex; shiftIndex + 1 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 1];
            }
            --argc;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--transparent-threshold" ||
               std::string(argv[argumentIndex]) == "-tt") {
            if (argumentIndex + 1 >= argc) {
                std::cerr << "Transparent threshold must be an integer from 0 to 255." << std::endl;
                return -1;
            }
            try {
                std::size_t parsedLength = 0;
                const std::string thresholdArgument = argv[argumentIndex + 1];
                transparentBrightnessThreshold = std::stoi(thresholdArgument, &parsedLength);
                if (parsedLength != thresholdArgument.length() ||
                    transparentBrightnessThreshold < 0 || transparentBrightnessThreshold > 255) {
                    throw std::invalid_argument("transparent threshold");
                }
            } catch (const std::exception&) {
                std::cerr << "Transparent threshold must be an integer from 0 to 255." << std::endl;
                return -1;
            }
            for (int shiftIndex = argumentIndex; shiftIndex + 2 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 2];
            }
            argc -= 2;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--glow-strength" ||
               std::string(argv[argumentIndex]) == "-gs") {
            if (argumentIndex + 1 >= argc) {
                std::cerr << "Glow strength must be a non-negative number." << std::endl;
                return -1;
            }
            try {
                std::size_t parsedLength = 0;
                const std::string strengthArgument = argv[argumentIndex + 1];
                glowStrength = std::stod(strengthArgument, &parsedLength);
                if (parsedLength != strengthArgument.length() ||
                    glowStrength < 0.0 || !std::isfinite(glowStrength)) {
                    throw std::invalid_argument("glow strength");
                }
            } catch (const std::exception&) {
                std::cerr << "Glow strength must be a non-negative number." << std::endl;
                return -1;
            }
            for (int shiftIndex = argumentIndex; shiftIndex + 2 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 2];
            }
            argc -= 2;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--glow-falloff" ||
               std::string(argv[argumentIndex]) == "-gf") {
            if (argumentIndex + 1 >= argc) {
                std::cerr << "Glow falloff must be a non-negative number." << std::endl;
                return -1;
            }
            try {
                std::size_t parsedLength = 0;
                const std::string falloffArgument = argv[argumentIndex + 1];
                glowFalloff = std::stod(falloffArgument, &parsedLength);
                if (parsedLength != falloffArgument.length() ||
                    glowFalloff < 0.0 || !std::isfinite(glowFalloff)) {
                    throw std::invalid_argument("glow falloff");
                }
            } catch (const std::exception&) {
                std::cerr << "Glow falloff must be a non-negative number." << std::endl;
                return -1;
            }
            for (int shiftIndex = argumentIndex; shiftIndex + 2 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 2];
            }
            argc -= 2;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--glow-resolution" ||
               std::string(argv[argumentIndex]) == "-gr") {
            if (argumentIndex + 1 >= argc) {
                std::cerr << "Glow resolution must be a number from 0.1 to 1.0." << std::endl;
                return -1;
            }
            try {
                std::size_t parsedLength = 0;
                const std::string resolutionArgument = argv[argumentIndex + 1];
                glowResolution = std::stod(resolutionArgument, &parsedLength);
                if (parsedLength != resolutionArgument.length() ||
                    glowResolution < 0.1 || glowResolution > 1.0 ||
                    !std::isfinite(glowResolution)) {
                    throw std::invalid_argument("glow resolution");
                }
            } catch (const std::exception&) {
                std::cerr << "Glow resolution must be a number from 0.1 to 1.0." << std::endl;
                return -1;
            }
            for (int shiftIndex = argumentIndex; shiftIndex + 2 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 2];
            }
            argc -= 2;
            --argumentIndex;
        } else if (std::string(argv[argumentIndex]) == "--glow-radius" ||
               std::string(argv[argumentIndex]) == "-grd") {
            if (argumentIndex + 1 >= argc) {
                std::cerr << "Glow radius must be a non-negative integer." << std::endl;
                return -1;
            }
            try {
                std::size_t parsedLength = 0;
                const std::string radiusArgument = argv[argumentIndex + 1];
                glowRadius = std::stoi(radiusArgument, &parsedLength);
                if (parsedLength != radiusArgument.length() || glowRadius < 0) {
                    throw std::invalid_argument("glow radius");
                }
            } catch (const std::exception&) {
                std::cerr << "Glow radius must be a non-negative integer." << std::endl;
                return -1;
            }
            for (int shiftIndex = argumentIndex; shiftIndex + 2 < argc; ++shiftIndex) {
                argv[shiftIndex] = argv[shiftIndex + 2];
            }
            argc -= 2;
            --argumentIndex;
        }
    }
    std::string videoPath = argc >= 2 ? argv[1] : "";
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
        std::cerr << "Usage: ascii-translation.exe [video-path] [red] [green] [blue] [-t|-o] [--transparent-threshold|-tt value] [--glow|-g] [--glow-strength|-gs value] [--glow-falloff|-gf value] [--glow-resolution|-gr value] [--glow-radius|-grd value]" << std::endl;
        return -1;
    }

    RenderSettings settings;
    settings.transparent = transparent;
    settings.glowEnabled = enableGlow;
    settings.transparentThreshold = transparentBrightnessThreshold;
    settings.red = textColor[2];
    settings.green = textColor[1];
    settings.blue = textColor[0];
    settings.glowStrength = static_cast<int>(std::round(glowStrength * 10.0));
    settings.glowFalloff = static_cast<int>(std::round(glowFalloff * 10.0));
    settings.glowResolution = static_cast<int>(std::round(glowResolution * 100.0));
    settings.glowRadius = glowRadius;
    SettingsWindow settingsWindow(settings);

    cv::VideoCapture cap;
    while (videoPath.empty() && !settingsWindow.shouldQuit()) {
        settingsWindow.update();
        settingsWindow.takeDroppedFile(videoPath);
        cv::waitKey(30);
    }
    if (settingsWindow.shouldQuit()) return 0;

    // Open the initial video, or wait for a valid dropped file.
    while (!cap.isOpened() && !settingsWindow.shouldQuit()) {
        cap.open(videoPath);
        if (cap.isOpened()) break;
        std::cerr << "Error: Could not open video file: " << videoPath << std::endl;
        videoPath.clear();
        settingsWindow.update();
        settingsWindow.takeDroppedFile(videoPath);
        cv::waitKey(30);
    }
    if (settingsWindow.shouldQuit()) return 0;
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video file: " << videoPath << std::endl;
        return -1;
    }

    // Target width for the window display (adjust based on your screen size)
    int targetWidth = settings.asciiResolution;
    
    // Terminal characters are taller than they are wide (usually a 1:2 aspect ratio).
    // We adjust the height scale so the ASCII output doesn't look vertically stretched.
    int videoWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int videoHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    if (videoWidth <= 0 || videoHeight <= 0) {
        std::cerr << "Error: Video dimensions could not be read." << std::endl;
        return -1;
    }
    double videoAspectRatio = static_cast<double>(videoWidth) / videoHeight;
    settingsWindow.setVideoAspectRatio(videoAspectRatio);
    settings.videoDisplayWidth = std::clamp(
        static_cast<int>(std::round(settings.videoDisplayHeight * videoAspectRatio)), 180, 1600);
    int targetHeight = std::max(1, static_cast<int>(targetWidth / (videoAspectRatio * 2.0)));

    cv::Mat frame, grayFrame, resizedFrame, asciiImage, glowImage;

    const int FONT_FACE = cv::FONT_HERSHEY_PLAIN;
    const double FONT_SCALE = 1.0;
    const int FONT_THICKNESS = 1;
    const int CHAR_WIDTH = 10;
    const int LINE_HEIGHT = 14;
    const int BASELINE = 3;
    int windowHeight = settings.videoDisplayHeight;
    int windowWidth = settings.videoDisplayWidth;

    std::vector<cv::Mat> glyphs;
    std::vector<cv::Mat> glyphMasks;
    std::array<unsigned char, 256> glyphIndexByGray{};
    for (int gray = 0; gray < 256; ++gray) {
        glyphIndexByGray[gray] = static_cast<unsigned char>(gray *
            (static_cast<int>(ASCII_CHARS.length()) - 1) / 255);
    }
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

    auto renderFrame = [&](const cv::Mat& sourceFrame, cv::Mat& outputImage) {
        cv::resize(sourceFrame, resizedFrame, cv::Size(targetWidth, targetHeight));
        cv::cvtColor(resizedFrame, grayFrame, cv::COLOR_BGR2GRAY);
        outputImage.create(targetHeight * LINE_HEIGHT + BASELINE,
            targetWidth * CHAR_WIDTH, CV_8UC3);
        outputImage.setTo(cv::Scalar(0, 0, 0));

        for (int y = 0; y < grayFrame.rows; ++y) {
            for (int x = 0; x < grayFrame.cols; ++x) {
                const int gray = grayFrame.ptr<uchar>(y)[x];
                if (transparent && gray <= transparentBrightnessThreshold) continue;
                const int glyphIndex = glyphIndexByGray[gray];
                cv::Mat glyphRegion = outputImage(
                    cv::Rect(x * CHAR_WIDTH, y * LINE_HEIGHT, CHAR_WIDTH, LINE_HEIGHT));
                glyphs[glyphIndex].copyTo(glyphRegion, glyphMasks[glyphIndex]);
            }
        }

        if (enableGlow) {
            const cv::Size glowSize(
                std::max(1, static_cast<int>(outputImage.cols * glowResolution)),
                std::max(1, static_cast<int>(outputImage.rows * glowResolution)));
            cv::resize(outputImage, glowImage, glowSize, 0.0, 0.0, cv::INTER_AREA);
            const int blurRadius = static_cast<int>(std::ceil(glowRadius * glowResolution));
            const cv::Size blurKernel = blurRadius > 0
                ? cv::Size(blurRadius * 2 + 1, blurRadius * 2 + 1)
                : cv::Size(0, 0);
            cv::GaussianBlur(glowImage, glowImage, blurKernel, glowFalloff / glowResolution);
            cv::resize(glowImage, glowImage, outputImage.size(), 0.0, 0.0, cv::INTER_LINEAR);
            cv::addWeighted(outputImage, 1.0, glowImage, glowStrength, 0.0, outputImage);
        }
    };

    TransparentWindow window("ASCII Video", windowWidth, windowHeight, transparent);
    if (!window.isOpen()) {
        std::cerr << "Error: Could not create the transparent display window." << std::endl;
        return -1;
    }

    // Get the frame rate of the original video to approximate playback speed
    double fps = cap.get(cv::CAP_PROP_FPS);
    if (!(fps > 0.0) || !std::isfinite(fps)) fps = 30.0;
    auto framePeriod = std::chrono::duration<double>(1.0 / fps);
    auto nextFrameDeadline = std::chrono::steady_clock::now();
    bool isPaused = false;
    int appliedWindowWidth = windowWidth;
    int appliedWindowHeight = windowHeight;
    bool appliedTransparency = transparent;

    while (true) {
        settingsWindow.update();
        if (settingsWindow.shouldQuit()) break;
        std::string droppedFile;
        if (settingsWindow.takeDroppedFile(droppedFile)) {
            cv::VideoCapture replacement(droppedFile);
            const int replacementWidth = static_cast<int>(replacement.get(cv::CAP_PROP_FRAME_WIDTH));
            const int replacementHeight = static_cast<int>(replacement.get(cv::CAP_PROP_FRAME_HEIGHT));
            if (replacement.isOpened() && replacementWidth > 0 && replacementHeight > 0) {
                cap = std::move(replacement);
                videoPath = droppedFile;
                videoWidth = replacementWidth;
                videoHeight = replacementHeight;
                videoAspectRatio = static_cast<double>(videoWidth) / videoHeight;
                settingsWindow.setVideoAspectRatio(videoAspectRatio);
                settings.videoDisplayWidth = std::clamp(
                    static_cast<int>(std::round(settings.videoDisplayHeight * videoAspectRatio)), 180, 1600);
                targetHeight = std::max(1, static_cast<int>(targetWidth / (videoAspectRatio * 2.0)));
                windowWidth = settings.videoDisplayWidth;
                windowHeight = settings.videoDisplayHeight;
                window.setSize(windowWidth, windowHeight);
                fps = cap.get(cv::CAP_PROP_FPS);
                if (!(fps > 10.0) || !std::isfinite(fps)) fps = 30.0;
                framePeriod = std::chrono::duration<double>(1.0 / fps);
                nextFrameDeadline = std::chrono::steady_clock::now();
            }
        }
        targetWidth = std::max(40, settings.asciiResolution);
        targetHeight = std::max(1, static_cast<int>(targetWidth / (videoAspectRatio * 2.0)));
        windowWidth = std::max(180, static_cast<int>(std::round(
            settings.videoDisplayWidth * settings.videoScale / 100.0)));
        windowHeight = std::max(180, static_cast<int>(std::round(
            settings.videoDisplayHeight * settings.videoScale / 100.0)));
        if (windowWidth != appliedWindowWidth || windowHeight != appliedWindowHeight) {
            window.setSize(windowWidth, windowHeight);
            appliedWindowWidth = windowWidth;
            appliedWindowHeight = windowHeight;
        }
        transparent = settings.transparent;
        enableGlow = settings.glowEnabled;
        transparentBrightnessThreshold = settings.transparentThreshold;
        glowStrength = settings.glowStrengthValue();
        glowFalloff = settings.glowFalloffValue();
        glowResolution = settings.glowResolutionValue();
        glowRadius = settings.glowRadius;
        if (transparent != appliedTransparency) {
            window.setTransparent(transparent);
            appliedTransparency = transparent;
        }

        const cv::Scalar updatedTextColor = settings.color();
        if (updatedTextColor[0] != textColor[0] || updatedTextColor[1] != textColor[1] ||
            updatedTextColor[2] != textColor[2]) {
            textColor = updatedTextColor;
            for (int glyphIndex = 0; glyphIndex < static_cast<int>(ASCII_CHARS.length()); ++glyphIndex) {
                const std::string character(1, ASCII_CHARS[glyphIndex]);
                glyphs[glyphIndex].setTo(cv::Scalar(0, 0, 0));
                cv::putText(glyphs[glyphIndex], character, cv::Point(0, LINE_HEIGHT - BASELINE),
                    FONT_FACE, FONT_SCALE, textColor, FONT_THICKNESS, cv::LINE_AA);
            }
        }

        std::string exportPath;
        if (settingsWindow.takeExportRequest(exportPath)) {
            const double currentFrame = cap.get(cv::CAP_PROP_POS_FRAMES);
            cv::VideoCapture exportCapture(videoPath);
            cv::VideoWriter writer;
            bool exportSucceeded = false;
            if (exportCapture.isOpened()) {
                exportCapture.set(cv::CAP_PROP_POS_FRAMES, 0);
                if (exportCapture.read(frame)) {
                    renderFrame(frame, asciiImage);
                    writer.open(exportPath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
                        fps, asciiImage.size(), true);
                    if (writer.isOpened()) {
                        exportSucceeded = true;
                        do {
                            renderFrame(frame, asciiImage);
                            writer.write(asciiImage);
                        } while (exportCapture.read(frame));
                    }
                }
            }
            writer.release();
            exportCapture.release();
            cap.set(cv::CAP_PROP_POS_FRAMES, currentFrame);
            if (exportSucceeded) {
                std::cerr << "Exported video to: " << exportPath << std::endl;
            } else {
                std::cerr << "Error: Could not write video export: " << exportPath << std::endl;
            }
        }

        if (!isPaused) {
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
        }

        renderFrame(frame, asciiImage);

        // 4. Render to the standalone OpenCV window
        window.show(asciiImage);

        // Maintain playback frame rate and allow the user to quit with Esc or Q
        nextFrameDeadline += std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod);
        const auto now = std::chrono::steady_clock::now();
        if (now > nextFrameDeadline) nextFrameDeadline = now;
        const auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextFrameDeadline - now);
        int key = window.waitKey(isPaused ? 30 : std::max(1, static_cast<int>(waitTime.count())));
        if (key == 27 || key == 'q' || key == 'Q') break;
        if (!window.isOpen()) break;
        if (key == ' ') {
            isPaused = !isPaused;
            if (!isPaused) nextFrameDeadline = std::chrono::steady_clock::now();
        }
    }

    return 0;
}
