#include "utils.hpp"

#include "config.hpp"

#include <QDebug>
#include <QProcess>
#include <QScopeGuard>

#if _WIN32 // Windows

#    include <Windows.h>
#    include <string>
#    include <tchar.h>

constexpr const UINT CLASSNAME_SIZE = 256;

static HWND findInkscapeWindow() {
    struct EnumWindowsParam {
        std::string className;
        std::string windowName;
        HWND inkscape = nullptr;
    } enumWindowsParam;

    // This lambda cannot capture anything, due to windows' CALLBACK
    // restrictions
    auto isNotInkscapeWindow = [](HWND window, LPARAM lParam) -> BOOL {
        EnumWindowsParam *param = (EnumWindowsParam *)lParam;
        param->inkscape = window;

        // Get class name
        param->className.clear();
        param->className.reserve(CLASSNAME_SIZE + 1);
        GetClassName(window, param->className.data(), CLASSNAME_SIZE);

        // get window name
        int winNameLen = GetWindowTextLength(window);
        param->windowName.clear();
        param->windowName.reserve(winNameLen * 4 + 1);
        GetWindowText(window, param->windowName.data(), winNameLen * 4 + 1);

        return !(
            QString(param->className.c_str())
                .toLower()
                .contains("gdkwindowtoplevel")
            && QString(param->windowName.c_str())
                   .toLower()
                   .contains("inkscape"));
    };

    // Test cached window first for performance
    static HWND cachedInkscapeWindow = nullptr;
    if (!isNotInkscapeWindow(cachedInkscapeWindow, (LPARAM)&enumWindowsParam))
        return cachedInkscapeWindow;

    // Try to find
    bool inkscapeFound =
        !EnumWindows(isNotInkscapeWindow, (LPARAM)&enumWindowsParam);
    if (inkscapeFound) {
        cachedInkscapeWindow = enumWindowsParam.inkscape;
        return enumWindowsParam.inkscape;
    }
    return nullptr;
}

void Utils::pasteToInkscape() {
    if (HWND inkscape = findInkscapeWindow(); inkscape) {
        qDebug() << "Pasting style to" << inkscape;

        // Windows assume that keyboard input goes to the foreground window,
        // and seems there's not way to send key combinations to a background
        // window.
        if (!SetForegroundWindow(inkscape)) {
            qInfo() << "Cannot bring inkscape window to foreground";
            return;
        }

        INPUT inputs[6] = {};
        ZeroMemory(inputs, sizeof(inputs));

        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = VK_SHIFT;
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 0x56;

        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = 0x56;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[4].type = INPUT_KEYBOARD;
        inputs[4].ki.wVk = VK_SHIFT;
        inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[5].type = INPUT_KEYBOARD;
        inputs[5].ki.wVk = VK_CONTROL;
        inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    } else {
        qInfo() << "Inkscape window not found";
    }
}

#else // Linux

#    include <X11/Xlib.h>
#    include <X11/Xutil.h>

static Window findInkscapeWindow(Display *display) {
    auto isInkscapeWindow = [](Display *display, Window window) -> bool {
        QString wmClass, wmName;

        // Get class hint
        XClassHint classHint = {nullptr, nullptr};
        if (!XGetClassHint(display, window, &classHint))
            return false;
        if (classHint.res_class) {
            wmClass = classHint.res_class;
            XFree(classHint.res_class);
        }
        if (classHint.res_name) {
            wmName = classHint.res_name;
            XFree(classHint.res_name);
        }

        // Get window attributes
        XWindowAttributes attrs;
        if (!XGetWindowAttributes(display, window, &attrs))
            return false;

        return (wmName.contains("inkscape") && attrs.map_state != IsUnmapped);
    };

    // Test cached window first for performance
    static Window cachedInkscapeWindow = 0;
    if (isInkscapeWindow(display, cachedInkscapeWindow))
        return cachedInkscapeWindow;

    // If cached window is not an inkscape window, try to find one
    Window root = XDefaultRootWindow(display), parent;
    Window *children = nullptr;
    auto cleanup = qScopeGuard([&] {
        if (children)
            XFree(children);
    });

    unsigned n;
    XQueryTree(display, root, &root, &parent, &children, &n);
    if (!children)
        return 0;

    // Search only the top-level windows
    for (unsigned i = 0; i < n; ++i) {
        if (isInkscapeWindow(display, children[i])) {
            cachedInkscapeWindow = children[i];
            return children[i];
        }
    }
    return 0;
}

void Utils::pasteToInkscape() {
    Display *display = XOpenDisplay(nullptr);
    auto cleanup = qScopeGuard([&] {
        if (display)
            XCloseDisplay(display);
    });

    if (Window inkscape = findInkscapeWindow(display); inkscape) {
        qDebug() << "Pasting style to" << inkscape;

        // Press and release Ctrl-Shift-v
        XKeyEvent event;
        event.display = display;
        event.window = inkscape;
        event.root = XDefaultRootWindow(display);
        event.subwindow = None;
        event.time = CurrentTime;
        event.x = 0;
        event.y = 0;
        event.x_root = 0;
        event.y_root = 0;
        event.same_screen = True;
        event.keycode = XKeysymToKeycode(display, XK_v);
        event.state = ControlMask | ShiftMask;

        event.type = KeyPress;
        XSendEvent(display, inkscape, False, None, (XEvent *)&event);

        event.type = KeyRelease;
        XSendEvent(display, inkscape, False, None, (XEvent *)&event);

        XFlush(display);
    } else {
        qInfo() << "Inkscape window not found" << inkscape;
    }
}
#endif
