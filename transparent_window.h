#pragma once

#include <windows.h>

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include <algorithm>
#include <cstring>
#include <string>

#include <opencv2/opencv.hpp>

class TransparentWindow {
public:
    TransparentWindow(const char* title, int width, int height, bool transparent)
        : title_(title), width_(width), height_(height), hwnd_(nullptr), bitmap_(nullptr),
                bitmapBits_(nullptr), screenDc_(nullptr), memoryDc_(nullptr),
                bitmapWidth_(0), bitmapHeight_(0), transparent_(transparent), fullscreen_(false),
                open_(false), pressedKey_(-1), spaceWasDown_(false), restoreRect_{} {
        static const char* className = "AsciiTransparentWindow";
        static bool classRegistered = false;
        if (!classRegistered) {
            WNDCLASSA windowClass{};
            windowClass.lpfnWndProc = windowProcedure;
            windowClass.hInstance = GetModuleHandleA(nullptr);
            windowClass.lpszClassName = className;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClassA(&windowClass);
            classRegistered = true;
        }

        hwnd_ = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TOOLWINDOW,
            className,
            title_.c_str(),
            WS_POPUP | WS_THICKFRAME,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            width_,
            height_,
            nullptr,
            nullptr,
            GetModuleHandleA(nullptr),
            this);
        open_ = hwnd_ != nullptr;
        if (open_) {
            ShowWindow(hwnd_, SW_SHOW);
            UpdateWindow(hwnd_);
            SetForegroundWindow(hwnd_);
        }
    }

    ~TransparentWindow() {
        if (memoryDc_ != nullptr) {
            DeleteDC(memoryDc_);
        }
        if (bitmap_ != nullptr) DeleteObject(bitmap_);
        if (screenDc_ != nullptr) ReleaseDC(nullptr, screenDc_);
        if (hwnd_ != nullptr) DestroyWindow(hwnd_);
    }

    bool isOpen() const {
        return open_;
    }

    void setTransparent(bool transparent) {
        if (transparent == transparent_) return;
        transparent_ = transparent;
    }

    void setSize(int width, int height) {
        if (width <= 0 || height <= 0) return;
        if (width == width_ && height == height_) return;
        width_ = width;
        height_ = height;
        if (hwnd_ != nullptr) {
            SetWindowPos(hwnd_, nullptr, 0, 0, width_, height_,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
        }
    }

    void show(const cv::Mat& image) {
        if (!open_ || image.empty()) return;

        resized_.create(height_, width_, CV_8UC3);
        if (image.cols == width_ && image.rows == height_) {
            image.copyTo(resized_);
        } else {
            cv::resize(image, resized_, cv::Size(width_, height_), 0.0, 0.0, cv::INTER_LINEAR);
        }
        cv::rectangle(resized_, cv::Rect(width_ - 40, 0, 40, 36), cv::Scalar(32, 32, 32), cv::FILLED);
        cv::line(resized_, cv::Point(width_ - 27, 11), cv::Point(width_ - 13, 25),
            cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        cv::line(resized_, cv::Point(width_ - 13, 11), cv::Point(width_ - 27, 25),
            cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        bgra_.create(height_, width_, CV_8UC4);
        cv::cvtColor(resized_, bgra_, cv::COLOR_BGR2BGRA);
        if (transparent_) {
            for (int y = 0; y < bgra_.rows; ++y) {
                cv::Vec4b* row = bgra_.ptr<cv::Vec4b>(y);
                for (int x = 0; x < bgra_.cols; ++x) {
                    row[x][3] = row[x][0] > row[x][1] ? row[x][0] : row[x][1];
                    row[x][3] = row[x][3] > row[x][2] ? row[x][3] : row[x][2];
                }
            }
        }

        if (!ensureBitmap()) return;
        std::memcpy(bitmapBits_, bgra_.data, static_cast<size_t>(width_) * height_ * 4);

        POINT destination{};
        RECT windowRect{};
        GetWindowRect(hwnd_, &windowRect);
        destination.x = windowRect.left;
        destination.y = windowRect.top;
        SIZE windowSize{width_, height_};
        POINT source{};
        BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
        UpdateLayeredWindow(hwnd_, screenDc_, &destination, &windowSize, memoryDc_,
            &source, 0, &blend, ULW_ALPHA);
    }

    int waitKey(int timeoutMilliseconds) {
        pressedKey_ = -1;
        const DWORD start = GetTickCount();
        MSG message{};
        do {
            while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) {
                    open_ = false;
                    return 27;
                }
                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
            const bool spaceDown = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
            if (spaceDown && !spaceWasDown_) pressedKey_ = VK_SPACE;
            spaceWasDown_ = spaceDown;
            if (pressedKey_ != -1 || !open_) break;
            if (timeoutMilliseconds <= 0) break;
            Sleep(1);
        } while (GetTickCount() - start < static_cast<DWORD>(timeoutMilliseconds));
        return pressedKey_;
    }

private:
    bool ensureBitmap() {
        if (bitmap_ != nullptr && bitmapWidth_ == width_ && bitmapHeight_ == height_) return true;
        if (screenDc_ == nullptr) screenDc_ = GetDC(nullptr);
        if (memoryDc_ == nullptr && screenDc_ != nullptr) memoryDc_ = CreateCompatibleDC(screenDc_);
        if (screenDc_ == nullptr || memoryDc_ == nullptr) return false;

        BITMAPINFO bitmapInfo{};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = width_;
        bitmapInfo.bmiHeader.biHeight = -height_;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        void* newBitmapBits = nullptr;
        HBITMAP newBitmap = CreateDIBSection(screenDc_, &bitmapInfo, DIB_RGB_COLORS,
            &newBitmapBits, nullptr, 0);
        if (newBitmap == nullptr) return false;

        SelectObject(memoryDc_, newBitmap);
        if (bitmap_ != nullptr) DeleteObject(bitmap_);
        bitmap_ = newBitmap;
        bitmapBits_ = newBitmapBits;
        bitmapWidth_ = width_;
        bitmapHeight_ = height_;
        return true;
    }

    void toggleFullscreen() {
        if (fullscreen_) {
            SetWindowLongPtrA(hwnd_, GWL_STYLE, WS_POPUP | WS_THICKFRAME);
            SetWindowPos(hwnd_, nullptr, restoreRect_.left, restoreRect_.top,
                restoreRect_.right - restoreRect_.left,
                restoreRect_.bottom - restoreRect_.top,
                SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            fullscreen_ = false;
            return;
        }

        GetWindowRect(hwnd_, &restoreRect_);
        const HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo{sizeof(MONITORINFO)};
        GetMonitorInfoA(monitor, &monitorInfo);
        SetWindowLongPtrA(hwnd_, GWL_STYLE, WS_POPUP);
        SetWindowPos(hwnd_, HWND_TOP,
            monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        fullscreen_ = true;
    }

    static LRESULT CALLBACK windowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        TransparentWindow* window = reinterpret_cast<TransparentWindow*>(
            GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const CREATESTRUCTA* createStruct = reinterpret_cast<const CREATESTRUCTA*>(lParam);
            window = static_cast<TransparentWindow*>(createStruct->lpCreateParams);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        if (window == nullptr) return DefWindowProcA(hwnd, message, wParam, lParam);

        if (message == WM_KEYDOWN) {
            if (wParam == VK_F11) {
                window->toggleFullscreen();
                return 0;
            }
            window->pressedKey_ = static_cast<int>(wParam);
            return 0;
        }
        if (message == WM_LBUTTONUP) {
            const int x = static_cast<short>(LOWORD(lParam));
            const int y = static_cast<short>(HIWORD(lParam));
            if (x >= window->width_ - 40 && y < 36) {
                DestroyWindow(hwnd);
                return 0;
            }
        }
        if (message == WM_SIZE) {
            const int newWidth = static_cast<int>(LOWORD(lParam));
            const int newHeight = static_cast<int>(HIWORD(lParam));
            if (newWidth > 0 && newHeight > 0) {
                window->width_ = newWidth;
                window->height_ = newHeight;
            }
        }
        if (message == WM_GETMINMAXINFO) {
            MINMAXINFO* sizeInfo = reinterpret_cast<MINMAXINFO*>(lParam);
            sizeInfo->ptMinTrackSize.x = 320;
            sizeInfo->ptMinTrackSize.y = 180;
            return 0;
        }
        if (message == WM_CLOSE || message == WM_DESTROY) {
            window->open_ = false;
            if (message == WM_DESTROY) PostQuitMessage(0);
            return 0;
        }
        if (message == WM_NCHITTEST) {
            POINT point{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
            ScreenToClient(hwnd, &point);
            if (point.x >= window->width_ - 40 && point.y < 36) return HTCLIENT;
            const int edgeSize = 8;
            const bool left = point.x < edgeSize;
            const bool right = point.x >= window->width_ - edgeSize;
            const bool top = point.y < edgeSize;
            const bool bottom = point.y >= window->height_ - edgeSize;
            if (top && left) return HTTOPLEFT;
            if (top && right) return HTTOPRIGHT;
            if (bottom && left) return HTBOTTOMLEFT;
            if (bottom && right) return HTBOTTOMRIGHT;
            if (left) return HTLEFT;
            if (right) return HTRIGHT;
            if (top) return HTTOP;
            if (bottom) return HTBOTTOM;
            return HTCAPTION;
        }
        return DefWindowProcA(hwnd, message, wParam, lParam);
    }

    std::string title_;
    int width_;
    int height_;
    HWND hwnd_;
    HBITMAP bitmap_;
    void* bitmapBits_;
    HDC screenDc_;
    HDC memoryDc_;
    int bitmapWidth_;
    int bitmapHeight_;
    bool transparent_;
    bool fullscreen_;
    bool open_;
    int pressedKey_;
    bool spaceWasDown_;
    RECT restoreRect_;
    cv::Mat resized_;
    cv::Mat bgra_;
};
