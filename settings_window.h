#pragma once

#include <algorithm>
#include <string>

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <opencv2/opencv.hpp>

struct RenderSettings {
    int transparent = 1;
    int glowEnabled = 0;
    int transparentThreshold = 16;
    int asciiResolution = 150;
    int red = 255;
    int green = 80;
    int blue = 255;
    int glowStrength = 50;
    int glowFalloff = 50;
    int glowResolution = 50;
    int glowRadius = 0;
    int videoDisplayWidth = 780;
    int videoDisplayHeight = 780;
    int videoScale = 100;

    cv::Scalar color() const {
        return cv::Scalar(blue, green, red);
    }

    double glowStrengthValue() const {
        return glowStrength / 10.0;
    }

    double glowFalloffValue() const {
        return glowFalloff / 10.0;
    }

    double glowResolutionValue() const {
        return std::max(10, glowResolution) / 100.0;
    }
};

class SettingsWindow {
public:
    explicit SettingsWindow(RenderSettings& settings) : settings_(settings) {
        cv::namedWindow(windowName_, cv::WINDOW_NORMAL);
        cv::resizeWindow(windowName_, 620, 1010);
        cv::setMouseCallback(windowName_, handleMouse, this);
        settingsHwnd_ = FindWindowA(nullptr, windowName_);
        if (settingsHwnd_ != nullptr) {
            DragAcceptFiles(settingsHwnd_, TRUE);
            SetPropA(settingsHwnd_, propertyName_, this);
            originalWindowProc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(
                settingsHwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(dropWindowProcedure)));
        }
        if (settingsHwnd_ != nullptr) MoveWindow(settingsHwnd_, 100, 100, 620, 1010, TRUE);
        drawCheckboxes();
    }

    ~SettingsWindow() {
        if (settingsHwnd_ != nullptr) {
            DragAcceptFiles(settingsHwnd_, FALSE);
            if (originalWindowProc_ != nullptr) {
                SetWindowLongPtrA(settingsHwnd_, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(originalWindowProc_));
            }
            RemovePropA(settingsHwnd_, propertyName_);
        }
        cv::destroyWindow(windowName_);
    }

    void update() {
        if (repaintRequested_) drawCheckboxes();
        cv::waitKey(1);
    }

    bool takeDroppedFile(std::string& path) {
        if (droppedFile_.empty()) return false;
        path = droppedFile_;
        droppedFile_.clear();
        return true;
    }

    bool shouldQuit() const {
        return quitRequested_;
    }

    bool takeExportRequest(std::string& path) {
        if (!exportRequested_) return false;
        exportRequested_ = false;
        path = exportPath_;
        exportPath_.clear();
        return true;
    }

    void setVideoAspectRatio(double aspectRatio) {
        if (aspectRatio > 0.0 && aspectRatio != videoAspectRatio_) {
            videoAspectRatio_ = aspectRatio;
            repaintRequested_ = true;
        }
    }

private:
    static LRESULT CALLBACK dropWindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        SettingsWindow* window = static_cast<SettingsWindow*>(GetPropA(hwnd, propertyName_));
        if (message == WM_DROPFILES && window != nullptr) {
            HDROP drop = reinterpret_cast<HDROP>(wParam);
            char path[MAX_PATH]{};
            if (DragQueryFileA(drop, 0, path, MAX_PATH) > 0) {
                window->droppedFile_ = path;
                window->droppedFileLabel_ = path;
                window->repaintRequested_ = true;
            }
            DragFinish(drop);
            return 0;
        }
        if (window != nullptr && window->originalWindowProc_ != nullptr) {
            return CallWindowProcA(window->originalWindowProc_, hwnd, message, wParam, lParam);
        }
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }

    static void handleMouse(int event, int x, int y, int flags, void* userData) {
        if (userData == nullptr) return;
        SettingsWindow* window = static_cast<SettingsWindow*>(userData);
        const double scale = window->scale();
        if (event == cv::EVENT_MOUSEWHEEL) {
            window->updateSliderByWheel(x, y, scale, flags, cv::getMouseWheelDelta(flags));
            window->drawCheckboxes();
            return;
        }
        if (event == cv::EVENT_LBUTTONDOWN ||
            (event == cv::EVENT_MOUSEMOVE && window->sliderDragging_)) {
            if (event == cv::EVENT_LBUTTONDOWN) window->sliderDragging_ = true;
            window->updateSlider(x, y, scale);
            window->drawCheckboxes();
            return;
        }
        if (event == cv::EVENT_RBUTTONUP) {
            window->resetSlider(y, scale);
            window->drawCheckboxes();
            return;
        }
        if (event != cv::EVENT_LBUTTONUP) return;
        window->sliderDragging_ = false;
        if (y >= static_cast<int>(36 * scale) && y < static_cast<int>(66 * scale)) {
            window->settings_.transparent = window->settings_.transparent ? 0 : 1;
        } else if (y >= static_cast<int>(76 * scale) && y < static_cast<int>(106 * scale)) {
            window->settings_.glowEnabled = window->settings_.glowEnabled ? 0 : 1;
        } else if (x >= static_cast<int>(12 * scale) && x < static_cast<int>(408 * scale) &&
            y >= window->exportY() && y < window->quitY()) {
            window->requestExport();
        } else if (x >= static_cast<int>(12 * scale) && x < static_cast<int>(408 * scale) &&
            y >= window->quitY()) {
            window->quitRequested_ = true;
        }
        window->drawCheckboxes();
    }

    void drawCheckboxes() {
        const cv::Rect imageRect = cv::getWindowImageRect(windowName_);
        const int width = std::max(620, imageRect.width);
        const int height = std::max(1010, imageRect.height);
        const double uiScale = std::min(width / 620.0, height / 1010.0);
        cv::Mat panel(height, width, CV_8UC3, cv::Scalar(45, 45, 45));
        cv::putText(panel, "ASCII SETTINGS", cv::Point(scaled(12, uiScale), scaled(30, uiScale)),
            cv::FONT_HERSHEY_SIMPLEX, 0.65 * uiScale, cv::Scalar(90, 220, 120),
            std::max(1, scaled(1, uiScale)), cv::LINE_AA);
        drawCheckbox(panel, scaled(36, uiScale), settings_.transparent != 0, "Transparent background", uiScale);
        drawCheckbox(panel, scaled(76, uiScale), settings_.glowEnabled != 0, "Enable glow effect", uiScale);
        cv::rectangle(panel, cv::Rect(scaled(12, uiScale), scaled(126, uiScale),
            width - scaled(24, uiScale), scaled(42, uiScale)), cv::Scalar(120, 120, 120),
            std::max(1, scaled(1, uiScale)), cv::LINE_AA);
        cv::putText(panel, droppedFileLabel_.empty() ? "Drop a video file here" : droppedFileLabel_,
            cv::Point(scaled(24, uiScale), scaled(152, uiScale)), cv::FONT_HERSHEY_SIMPLEX,
            0.5 * uiScale, cv::Scalar(235, 235, 235), std::max(1, scaled(1, uiScale)), cv::LINE_AA);
        drawSliders(panel, uiScale);
        cv::rectangle(panel, cv::Rect(scaled(12, uiScale), exportY(), width - scaled(24, uiScale),
            scaled(40, uiScale)), cv::Scalar(70, 150, 100), cv::FILLED);
        cv::putText(panel, "EXPORT VIDEO", cv::Point(width / 2 - scaled(54, uiScale),
            exportY() + scaled(27, uiScale)), cv::FONT_HERSHEY_SIMPLEX, 0.6 * uiScale,
            cv::Scalar(255, 255, 255), std::max(1, scaled(1, uiScale)), cv::LINE_AA);
        cv::rectangle(panel, cv::Rect(scaled(12, uiScale), quitY(), width - scaled(24, uiScale),
            scaled(40, uiScale)), cv::Scalar(80, 80, 190), cv::FILLED);
        cv::putText(panel, "QUIT", cv::Point(width / 2 - scaled(24, uiScale), quitY() + scaled(27, uiScale)),
            cv::FONT_HERSHEY_SIMPLEX, 0.6 * uiScale, cv::Scalar(255, 255, 255),
            std::max(1, scaled(1, uiScale)), cv::LINE_AA);
        cv::imshow(windowName_, panel);
        repaintRequested_ = false;
    }

    static void drawCheckbox(cv::Mat& panel, int y, bool checked, const char* label, double scale) {
        const cv::Scalar boxColor(210, 210, 210);
        cv::rectangle(panel, cv::Rect(scaled(12, scale), y, scaled(22, scale), scaled(22, scale)),
            boxColor, std::max(1, scaled(1, scale)), cv::LINE_AA);
        if (checked) {
            cv::line(panel, cv::Point(scaled(16, scale), y + scaled(11, scale)),
                cv::Point(scaled(22, scale), y + scaled(17, scale)), cv::Scalar(90, 220, 120),
                std::max(1, scaled(2, scale)), cv::LINE_AA);
            cv::line(panel, cv::Point(scaled(22, scale), y + scaled(17, scale)),
                cv::Point(scaled(31, scale), y + scaled(5, scale)), cv::Scalar(90, 220, 120),
                std::max(1, scaled(2, scale)), cv::LINE_AA);
        }
        cv::putText(panel, label, cv::Point(scaled(46, scale), y + scaled(17, scale)),
            cv::FONT_HERSHEY_SIMPLEX, 0.55 * scale, cv::Scalar(235, 235, 235),
            std::max(1, scaled(1, scale)), cv::LINE_AA);
    }

    static int scaled(int value, double scale) {
        return std::max(1, static_cast<int>(value * scale));
    }

    double scale() const {
        const cv::Rect imageRect = cv::getWindowImageRect(windowName_);
        return std::min(std::max(620, imageRect.width) / 620.0,
            std::max(1010, imageRect.height) / 1010.0);
    }

    int quitY() const {
        return scaled(955, scale());
    }

    int exportY() const {
        return scaled(905, scale());
    }

    void requestExport() {
        char path[MAX_PATH] = "ascii-video.avi";
        OPENFILENAMEA dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = settingsHwnd_;
        dialog.lpstrFilter = "AVI video (*.avi)\0*.avi\0All files (*.*)\0*.*\0";
        dialog.lpstrFile = path;
        dialog.nMaxFile = MAX_PATH;
        dialog.lpstrDefExt = "avi";
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if (GetSaveFileNameA(&dialog)) {
            exportPath_ = path;
            exportRequested_ = true;
        }
    }

    void drawSliders(cv::Mat& panel, double scale) {
        const char* labels[] = {
            "ASCII resolution (columns)", "Video window scale (%)", "Video display width",
            "Video display height",
            "Transparent threshold", "Text color - red", "Text color - green",
            "Text color - blue", "Glow strength (x10)", "Glow falloff (x10)",
            "Glow resolution (%)", "Glow radius"
        };
        int* values[] = {&settings_.asciiResolution, &settings_.videoScale,
            &settings_.videoDisplayWidth, &settings_.videoDisplayHeight,
            &settings_.transparentThreshold, &settings_.red,
            &settings_.green, &settings_.blue, &settings_.glowStrength, &settings_.glowFalloff,
            &settings_.glowResolution, &settings_.glowRadius};
        const int maximums[] = {300, 300, 1600, 1200, 255, 255, 255, 255, 200, 200, 100, 100};
        const int minimums[] = {40, 25, 180, 180, 0, 0, 0, 0, 0, 0, 10, 0};
        for (int index = 0; index < 12; ++index) {
            const int y = scaled(220 + index * 58, scale);
            cv::putText(panel, labels[index], cv::Point(scaled(12, scale), y),
                cv::FONT_HERSHEY_SIMPLEX, 0.5 * scale, cv::Scalar(235, 235, 235),
                std::max(1, scaled(1, scale)), cv::LINE_AA);
            const int left = scaled(270, scale);
            const int right = panel.cols - scaled(20, scale);
            const int trackY = y - scaled(7, scale);
            cv::line(panel, cv::Point(left, trackY), cv::Point(right, trackY),
                cv::Scalar(120, 120, 120), std::max(2, scaled(4, scale)), cv::LINE_AA);
            const double fraction = (*values[index] - minimums[index]) /
                static_cast<double>(maximums[index] - minimums[index]);
            const int knobX = left + static_cast<int>(std::clamp(fraction, 0.0, 1.0) * (right - left));
            cv::circle(panel, cv::Point(knobX, trackY), scaled(9, scale),
                cv::Scalar(90, 220, 120), cv::FILLED, cv::LINE_AA);
        }
    }

    void updateSlider(int x, int y, double scale) {
        const int firstY = scaled(220, scale);
        const int rowHeight = scaled(58, scale);
        const int index = (y - firstY + rowHeight / 2) / rowHeight;
        if (index < 0 || index >= 12) return;
        int* values[] = {&settings_.asciiResolution, &settings_.videoScale,
            &settings_.videoDisplayWidth, &settings_.videoDisplayHeight,
            &settings_.transparentThreshold, &settings_.red,
            &settings_.green, &settings_.blue, &settings_.glowStrength, &settings_.glowFalloff,
            &settings_.glowResolution, &settings_.glowRadius};
        const int maximums[] = {300, 300, 1600, 1200, 255, 255, 255, 255, 200, 200, 100, 100};
        const int minimums[] = {40, 25, 180, 180, 0, 0, 0, 0, 0, 0, 10, 0};
        const int left = scaled(270, scale);
        const int right = static_cast<int>(std::max(620.0, windowWidth() / scale) * scale) -
            scaled(20, scale);
        if (x < left || x > right) return;
        *values[index] = minimums[index] + static_cast<int>((x - left) *
            (maximums[index] - minimums[index]) / static_cast<double>(right - left));
    }

    void resetSlider(int y, double scale) {
        const int firstY = scaled(220, scale);
        const int rowHeight = scaled(58, scale);
        const int index = (y - firstY + rowHeight / 2) / rowHeight;
        if (index < 0 || index >= 12) return;
        int* values[] = {&settings_.asciiResolution, &settings_.videoScale,
            &settings_.videoDisplayWidth, &settings_.videoDisplayHeight,
            &settings_.transparentThreshold, &settings_.red, &settings_.green, &settings_.blue,
            &settings_.glowStrength, &settings_.glowFalloff, &settings_.glowResolution,
            &settings_.glowRadius};
        const int defaults[] = {150, 100, 780, 780, 16, 255, 80, 255, 50, 50, 50, 0};
        if (index == 2) {
            *values[index] = std::clamp(static_cast<int>(std::round(
                defaults[3] * videoAspectRatio_)), 180, 1600);
        } else if (index == 3) {
            *values[index] = defaults[3];
        } else {
            *values[index] = defaults[index];
        }
    }

    void updateSliderByWheel(int x, int y, double scale, int flags, int wheelDelta) {
        const int firstY = scaled(220, scale);
        const int rowHeight = scaled(58, scale);
        const int index = (y - firstY + rowHeight / 2) / rowHeight;
        if (index < 0 || index >= 12 || wheelDelta == 0) return;
        const int left = scaled(270, scale);
        const int right = static_cast<int>(std::max(620.0, windowWidth() / scale) * scale) -
            scaled(20, scale);
        if (x < left || x > right) return;
        int* values[] = {&settings_.asciiResolution, &settings_.videoScale,
            &settings_.videoDisplayWidth, &settings_.videoDisplayHeight,
            &settings_.transparentThreshold, &settings_.red, &settings_.green,
            &settings_.blue, &settings_.glowStrength, &settings_.glowFalloff,
            &settings_.glowResolution, &settings_.glowRadius};
        const int maximums[] = {300, 300, 1600, 1200, 255, 255, 255, 255, 200, 200, 100, 100};
        const int minimums[] = {40, 25, 180, 180, 0, 0, 0, 0, 0, 0, 10, 0};
        const bool fineStep = (flags & cv::EVENT_FLAG_CTRLKEY) != 0;
        const bool glowTenths = index == 8 || index == 9;
        const int magnitude = (flags & cv::EVENT_FLAG_SHIFTKEY) ? 10 :
            (fineStep && glowTenths ? 1 : 1);
        const int step = wheelDelta > 0 ? magnitude : -magnitude;
        *values[index] = std::clamp(*values[index] + step, minimums[index], maximums[index]);
    }

    int windowWidth() const {
        const cv::Rect imageRect = cv::getWindowImageRect(windowName_);
        return std::max(620, imageRect.width);
    }

    static constexpr const char* windowName_ = "ASCII Settings";
    static constexpr const char* propertyName_ = "AsciiSettingsWindow";
    RenderSettings& settings_;
    HWND settingsHwnd_ = nullptr;
    WNDPROC originalWindowProc_ = nullptr;
    std::string droppedFile_;
    std::string droppedFileLabel_;
    bool quitRequested_ = false;
    bool exportRequested_ = false;
    std::string exportPath_;
    bool sliderDragging_ = false;
    bool repaintRequested_ = true;
    double videoAspectRatio_ = 1.0;
};