#ifndef _WIDOW_MANAGER_
#define _WIDOW_MANAGER_

#include "window/wayland_exporter.h"
#include "window/wayland_foreign.h"
#include <EGL/egl.h>

class WindowManager
{
    bool initWaylandEGLSurface();
    bool initEgl();
    bool adjustVideoRatio();

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

public:
    WindowManager();
    ~WindowManager();

    bool initialize();
    bool finalize();

    void setExporterRegion(int exporter_number, int x, int y, int w, int h);

    Wayland::Foreign foreign;
    Wayland::Exporter exporter1, exporter2;
};

#endif // _WIDOW_MANAGER_
