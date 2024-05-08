#ifndef _WIDOW_MANAGER_
#define _WIDOW_MANAGER_

#include "wayland/wayland_exporter.h"
#include "wayland/wayland_foreign.h"
#include <EGL/egl.h>

struct wl_Rect
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;

    bool operator==(const wl_Rect &other) const
    {
        return (x == other.x) && (y == other.y) && (w == other.w) && (h == other.h);
    }
    bool operator!=(const wl_Rect &other) const { return !(*this == other); }
};

enum class wl_State
{
    Idle,
    Normal,
    FullScreen
};

class WindowManager
{
    bool initWaylandEGLSurface();
    bool initEgl();
    bool adjustVideoRatio();
    void setExporterRegion(int, wl_Rect &);

    struct WaylandEGLSurface
    {
        struct wl_egl_window *native;
        struct wl_surface *wlSurface;
        struct wl_shell_surface *wlShellSurface;
        struct wl_webos_shell_surface *webosShellSurface;
        EGLSurface eglSurface;
        unsigned int width;
        unsigned int height;
    };

    WaylandEGLSurface surface;

    wl_Rect orgRect[2], rect[2];
    wl_State exporterState[2] = {wl_State::Idle, wl_State::Idle};

public:
    WindowManager();
    ~WindowManager();

    bool initialize();
    bool finalize();

    bool handleInput(int, int);
    bool isFullScreen();
    void setRect(int);
    void clearRect(int);
    void setFullscreen(int);
    void setNormalSize(int);

    Wayland::Foreign foreign;
    Wayland::Exporter exporter[2];
};

#endif // _WIDOW_MANAGER_
