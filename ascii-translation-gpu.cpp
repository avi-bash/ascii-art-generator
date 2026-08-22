#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>
#include "transparent_window.h"

const std::string ASCII_CHARS = ".:><-=+*#%@";

static const char* ASCII_RENDER_KERNEL = R"CLC(
__kernel void ascii_render(
    __global const uchar* src, int src_step, int src_offset,
    __global uchar* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
    __global const uchar* glyphs, int glyph_step, int glyph_offset,
    __global const uchar* masks, int mask_step, int mask_offset,
    int src_width, int src_height, int target_width, int target_height,
    int char_width, int line_height, int glyph_width, int glyph_height,
    int glyph_count)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    if (x >= dst_cols || y >= dst_rows) return;

    const int dst_index = y * dst_step + x * 3 + dst_offset;
    dst[dst_index + 0] = 0;
    dst[dst_index + 1] = 0;
    dst[dst_index + 2] = 0;

    const int content_width = target_width * char_width;
    const int content_height = target_height * line_height;
    if (x >= content_width || y >= content_height) return;

    const int cell_x = x / char_width;
    const int cell_y = y / line_height;
    const int source_x = min(src_width - 1, cell_x * src_width / target_width);
    const int source_y = min(src_height - 1, cell_y * src_height / target_height);
    const int source_index = source_y * src_step + source_x * 3 + src_offset;
    const int gray = (29 * (int)src[source_index + 0] +
                      150 * (int)src[source_index + 1] +
                      77 * (int)src[source_index + 2]) >> 8;
    const int glyph_index = gray * (glyph_count - 1) / 255;
    const int glyph_x = x % char_width;
    const int glyph_y = y % line_height;
    if (glyph_x >= glyph_width || glyph_y >= glyph_height) return;

    const int glyph_row = glyph_index * glyph_height + glyph_y;
    const int mask_index = glyph_row * mask_step + glyph_x + mask_offset;
    if (masks[mask_index] == 0) return;

    const int glyph_index_bytes = glyph_row * glyph_step + glyph_x * 3 + glyph_offset;
    dst[dst_index + 0] = glyphs[glyph_index_bytes + 0];
    dst[dst_index + 1] = glyphs[glyph_index_bytes + 1];
    dst[dst_index + 2] = glyphs[glyph_index_bytes + 2];
}
)CLC";

int main(int argc, char** argv) {
    bool enableGlow = false;
    bool transparent = true;
    double glowStrength = 5;
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
        std::cerr << "Usage: ascii-translation-gpu.exe [video-path] [red] [green] [blue] [-t|-o] [--glow|-g] [--glow-strength|-gs value]" << std::endl;
        return -1;
    }

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video file: " << videoPath << std::endl;
        return -1;
    }

    const int videoWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    const int videoHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    if (videoWidth <= 0 || videoHeight <= 0) {
        std::cerr << "Error: Video dimensions could not be read." << std::endl;
        return -1;
    }

    const int targetWidth = 150;
    const int targetHeight = std::max(1, static_cast<int>(
        targetWidth / (static_cast<double>(videoWidth) / videoHeight * 2.0)));
    const int charWidth = 10;
    const int lineHeight = 14;
    const int baseline = 3;
    const int outputWidth = targetWidth * charWidth;
    const int outputHeight = targetHeight * lineHeight + baseline;
    const int windowHeight = 780;
    const int windowWidth = static_cast<int>(windowHeight *
        static_cast<double>(videoWidth) / videoHeight);

    cv::ocl::setUseOpenCL(true);
    if (!cv::ocl::haveOpenCL() || cv::ocl::Context::getDefault().ndevices() == 0 ||
        !cv::ocl::Context::getDefault().device(0).compilerAvailable()) {
        std::cerr << "Error: A compiling OpenCL device is required for the full GPU renderer." << std::endl;
        return -1;
    }

    cv::Mat glyphAtlas(lineHeight * static_cast<int>(ASCII_CHARS.length()), charWidth,
        CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Mat maskAtlas(lineHeight * static_cast<int>(ASCII_CHARS.length()), charWidth,
        CV_8UC1, cv::Scalar(0));
    for (int glyphIndex = 0; glyphIndex < static_cast<int>(ASCII_CHARS.length()); ++glyphIndex) {
        const std::string character(1, ASCII_CHARS[glyphIndex]);
        cv::Mat glyphRegion = glyphAtlas.rowRange(glyphIndex * lineHeight, (glyphIndex + 1) * lineHeight);
        cv::Mat maskRegion = maskAtlas.rowRange(glyphIndex * lineHeight, (glyphIndex + 1) * lineHeight);
        cv::putText(glyphRegion, character, cv::Point(0, lineHeight - baseline),
            cv::FONT_HERSHEY_PLAIN, 1.0, textColor, 1, cv::LINE_AA);
        cv::putText(maskRegion, character, cv::Point(0, lineHeight - baseline),
            cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(255), 1, cv::LINE_AA);
    }

    cv::UMat gpuGlyphAtlas, gpuMaskAtlas;
    glyphAtlas.copyTo(gpuGlyphAtlas);
    maskAtlas.copyTo(gpuMaskAtlas);
    cv::Mat frame;
    cv::UMat gpuFrame, gpuAscii, gpuGlowSmall, gpuGlow;
    cv::Mat displayImage;

    const cv::ocl::ProgramSource programSource("ascii_gpu_full", "ascii_render", ASCII_RENDER_KERNEL, "");
    cv::ocl::Kernel renderKernel("ascii_render", programSource);
    if (renderKernel.empty()) {
        std::cerr << "Error: OpenCL ASCII render kernel could not be compiled." << std::endl;
        return -1;
    }

    TransparentWindow window("ASCII Video", windowWidth, windowHeight, transparent);
    if (!window.isOpen()) {
        std::cerr << "Error: Could not create the transparent display window." << std::endl;
        return -1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (!(fps > 0.0) || !std::isfinite(fps)) fps = 30.0;
    const auto framePeriod = std::chrono::duration<double>(1.0 / fps);
    auto nextFrameDeadline = std::chrono::steady_clock::now();
    bool paused = false;

    while (true) {
        if (paused) {
            const int key = window.waitKey(30);
            if (key == 32) {
                paused = false;
                nextFrameDeadline = std::chrono::steady_clock::now();
            }
            if (key == 27 || key == 'q' || key == 'Q') break;
            if (!window.isOpen()) break;
            continue;
        }

        while (std::chrono::steady_clock::now() > nextFrameDeadline +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod)) {
            if (!cap.grab()) {
                if (!cap.set(cv::CAP_PROP_POS_FRAMES, 0) || !cap.grab()) return 0;
            }
            nextFrameDeadline += std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod);
        }

        cap >> frame;
        if (frame.empty()) {
            if (!cap.set(cv::CAP_PROP_POS_FRAMES, 0)) break;
            cap >> frame;
            if (frame.empty()) break;
        }
        frame.copyTo(gpuFrame);
        gpuAscii.create(outputHeight, outputWidth, CV_8UC3);

        size_t globalSize[] = {static_cast<size_t>(outputWidth), static_cast<size_t>(outputHeight)};
        if (!renderKernel.args(
                cv::ocl::KernelArg::ReadOnlyNoSize(gpuFrame),
                cv::ocl::KernelArg::WriteOnly(gpuAscii),
                cv::ocl::KernelArg::ReadOnlyNoSize(gpuGlyphAtlas),
                cv::ocl::KernelArg::ReadOnlyNoSize(gpuMaskAtlas),
                videoWidth, videoHeight, targetWidth, targetHeight,
                charWidth, lineHeight, charWidth, lineHeight,
                static_cast<int>(ASCII_CHARS.length())).run(2, globalSize, nullptr, true)) {
            std::cerr << "Error: OpenCL ASCII render kernel failed." << std::endl;
            break;
        }

        if (enableGlow) {
            const cv::Size glowSize(std::max(1, outputWidth / 4), std::max(1, outputHeight / 4));
            cv::resize(gpuAscii, gpuGlowSmall, glowSize, 0.0, 0.0, cv::INTER_AREA);
            cv::GaussianBlur(gpuGlowSmall, gpuGlowSmall, cv::Size(0, 0), 5.0);
            cv::resize(gpuGlowSmall, gpuGlow, gpuAscii.size(), 0.0, 0.0, cv::INTER_LINEAR);
            cv::addWeighted(gpuAscii, 1.0, gpuGlow, glowStrength, 0.0, gpuAscii);
        }

        gpuAscii.copyTo(displayImage);
        window.show(displayImage);

        nextFrameDeadline += std::chrono::duration_cast<std::chrono::steady_clock::duration>(framePeriod);
        const auto now = std::chrono::steady_clock::now();
        if (now > nextFrameDeadline) nextFrameDeadline = now;
        const auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(nextFrameDeadline - now);
        const int key = window.waitKey(std::max(1, static_cast<int>(waitTime.count())));
        if (key == 32) paused = true;
        if (key == 27 || key == 'q' || key == 'Q') break;
        if (!window.isOpen()) break;
    }

    return 0;
}
