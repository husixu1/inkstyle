#include "utils.hpp"

#include "config.hpp"

#include <QDebug>
#include <QProcess>
#include <QScopeGuard>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#if _WIN32
void Utils::pasteToInkscape() {}
#else
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
        XSendEvent(display, inkscape, True, None, (XEvent *)&event);

        event.type = KeyRelease;
        XSendEvent(display, inkscape, True, None, (XEvent *)&event);

        XFlush(display);
        XSync(display, False);
    } else {
        qInfo() << "Inkscape window not found" << inkscape;
    }
}
#endif
