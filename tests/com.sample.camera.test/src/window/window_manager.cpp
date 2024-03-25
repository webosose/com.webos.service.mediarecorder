#include "window_manager.h"
#include "appParm.h"
#include "log_info.h"
#include <EGL/egl.h>
#include <cassert>
#include <iostream>

WindowManager::WindowManager() {}

WindowManager::~WindowManager() {}

bool WindowManager::initialize()
{
    foreign.initialize();
    initWaylandEGLSurface();
    adjustVideoRatio();
    initEgl();

    return true;
}

bool WindowManager::initWaylandEGLSurface()
{
    DEBUG_LOG("start");

    surface.wlSurface = wl_compositor_create_surface(foreign.getCompositor());
    surface.webosShellSurface =
        wl_webos_shell_get_shell_surface(foreign.getWebosShell(), surface.wlSurface);
    if (surface.webosShellSurface == NULL)
    {
        DEBUG_LOG("Can't create webos shell surface");
        exit(1);
    }

    wl_webos_shell_surface_set_property(surface.webosShellSurface, "displayAffinity",
                                        (getenv("DISPLAY_ID") ? getenv("DISPLAY_ID") : "0"));

    wl_webos_shell_surface_set_property(surface.webosShellSurface, "appId",
                                        "com.sample.camera.test");

    surface.width  = 1920;
    surface.height = 1080;

    uint32_t exported_type = WL_WEBOS_FOREIGN_WEBOS_EXPORTED_TYPE_VIDEO_OBJECT;

    for (int i = 0; i < 2; i++)
    {
        if (exporter[i].initialize(foreign.getDisplay(), foreign.getWebosForeign(),
                                   surface.wlSurface, exported_type))
        {
            std::cout << "exporter" << i << " initialize success" << std::endl;
        }
        else
        {
            std::cout << "exporter" << i << " initialize error" << std::endl;
        }

        std::cout << "exporter" << i << " window ID is : " << exporter[i].getWindowID()
                  << std::endl;
    }
    foreign.flush();

    return true;
}

bool WindowManager::initEgl()
{
    struct EGLData
    {
        EGLDisplay eglDisplay;
        EGLContext eglContext;
        EGLConfig *eglConfig;
        int configSelect;
        EGLConfig currentEglConfig;
    };

    EGLData eglData;
    int configs;

    int want_red   = 8;
    int want_green = 8;
    int want_blue  = 8;
    int want_alpha = 8;

    EGLint major_version;
    EGLint minor_version;

    eglData.eglDisplay = eglGetDisplay((EGLNativeDisplayType)foreign.getDisplay());
    eglInitialize(eglData.eglDisplay, &major_version, &minor_version);
    eglBindAPI(EGL_OPENGL_ES_API);
    eglGetConfigs(eglData.eglDisplay, NULL, 0, &configs);

    eglData.eglConfig = (EGLConfig *)alloca(configs * sizeof(EGLConfig));
    {
        const int NUM_ATTRIBS = 21;
        EGLint *attr          = (EGLint *)malloc(NUM_ATTRIBS * sizeof(EGLint));
        int i                 = 0;

        attr[i++] = EGL_RED_SIZE;
        attr[i++] = want_red;
        attr[i++] = EGL_GREEN_SIZE;
        attr[i++] = want_green;
        attr[i++] = EGL_BLUE_SIZE;
        attr[i++] = want_blue;
        attr[i++] = EGL_ALPHA_SIZE;
        attr[i++] = want_alpha;
        attr[i++] = EGL_DEPTH_SIZE;
        attr[i++] = 24;
        attr[i++] = EGL_STENCIL_SIZE;
        attr[i++] = 0;
        attr[i++] = EGL_SURFACE_TYPE;
        attr[i++] = EGL_WINDOW_BIT;
        attr[i++] = EGL_RENDERABLE_TYPE;
        attr[i++] = EGL_OPENGL_ES2_BIT;
        attr[i++] = EGL_NONE;

        assert(i <= NUM_ATTRIBS);

        if (!eglChooseConfig(eglData.eglDisplay, attr, eglData.eglConfig, configs, &configs) ||
            (configs == 0))
        {
            std::cout << "('w')/ eglChooseConfig() failed." << std::endl;
            return false;
        }

        free(attr);
    }

    for (eglData.configSelect = 0; eglData.configSelect < configs; eglData.configSelect++)
    {
        EGLint red_size, green_size, blue_size, alpha_size, depth_size;

        eglGetConfigAttrib(eglData.eglDisplay, eglData.eglConfig[eglData.configSelect],
                           EGL_RED_SIZE, &red_size);
        eglGetConfigAttrib(eglData.eglDisplay, eglData.eglConfig[eglData.configSelect],
                           EGL_GREEN_SIZE, &green_size);
        eglGetConfigAttrib(eglData.eglDisplay, eglData.eglConfig[eglData.configSelect],
                           EGL_BLUE_SIZE, &blue_size);
        eglGetConfigAttrib(eglData.eglDisplay, eglData.eglConfig[eglData.configSelect],
                           EGL_ALPHA_SIZE, &alpha_size);
        eglGetConfigAttrib(eglData.eglDisplay, eglData.eglConfig[eglData.configSelect],
                           EGL_DEPTH_SIZE, &depth_size);

        if ((red_size == want_red) && (green_size == want_green) && (blue_size == want_blue) &&
            (alpha_size == want_alpha))
        {
            break;
        }
    }

    if (eglData.configSelect == configs)
    {
        std::cout << "No suitable configs found." << std::endl;
        return false;
    }

    eglData.currentEglConfig = eglData.eglConfig[eglData.configSelect];

    EGLint ctx_attrib_list[3] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    ctx_attrib_list[1]        = 2; // Client Version.

    eglData.eglContext =
        eglCreateContext(eglData.eglDisplay, eglData.eglConfig[eglData.configSelect],
                         EGL_NO_CONTEXT, ctx_attrib_list);

    eglSwapInterval(eglData.eglDisplay, 1);

    surface.native     = wl_egl_window_create(surface.wlSurface, surface.width, surface.height);
    surface.eglSurface = eglCreateWindowSurface(eglData.eglDisplay, eglData.currentEglConfig,
                                                (EGLNativeWindowType)(surface.native), NULL);

    eglMakeCurrent(eglData.eglDisplay, surface.eglSurface, surface.eglSurface, eglData.eglContext);

    return true;
}

bool WindowManager::finalize()
{
    foreign.flush();

    for (int i = 0; i < 2; i++)
        exporter[i].finalize();

    foreign.finalize();

    return true;
}

void WindowManager::setExporterRegion(int e, wl_Rect &rect)
{
    DEBUG_LOG("[%d] %d %d %d %d", e, rect.x, rect.y, rect.w, rect.h);

    struct wl_region *region_src = foreign.createRegion(0, 0, 1920, 1080); // don't care
    struct wl_region *region_dst = foreign.createRegion(rect.x, rect.y, rect.w, rect.h);
    exporter[e].setRegion(region_src, region_dst);
}

bool WindowManager::adjustVideoRatio()
{
    // Camera Play
    // adjust video ratio
    int h = 720;
    int w = h * appParm.width / appParm.height;
    int x = (1920 - (w + w / 2)) / 2;
    int y = 0;

    appParm.x1 = x;
    appParm.x2 = x + w;

    DEBUG_LOG("Camera Play %d %d %d %d", x, y, w, h);
    struct wl_region *region_src  = foreign.createRegion(0, 0, 1920, 1080); // don't care
    struct wl_region *region_dst1 = foreign.createRegion(x, y, w, h);
    exporter[0].setRegion(region_src, region_dst1);
    orgRect[0] = {x, y, w, h};

    // video play
    DEBUG_LOG("Video Play %d %d %d %d", x + w, y, w / 2, h / 2);
    struct wl_region *region_dst2 = foreign.createRegion(x + w, y, w / 2, h / 2);
    exporter[1].setRegion(region_src, region_dst2);
    orgRect[1] = {x + w, y, w / 2, h / 2};

    return true;
}

bool WindowManager::isFullScreen()
{
    return (exporterState[0] == wl_State::FullScreen) || (exporterState[1] == wl_State::FullScreen);
}

void WindowManager::setFullscreen(int i)
{
    printf("[%d] %s\n", i, __func__);

    // adjust video ratio
    int h = 1080;
    int w = h * appParm.width / appParm.height;
    int x = (1920 - w) / 2;
    int y = 0;

    rect[i] = {x, y, w, h};
    setExporterRegion(i, rect[i]);
    exporterState[i] = wl_State::FullScreen;
}

void WindowManager::setNormalSize(int i)
{
    printf("[%d] %s\n", i, __func__);

    rect[i] = orgRect[i];
    setExporterRegion(i, rect[i]);
    exporterState[i] = wl_State::Normal;
}

void WindowManager::setRect(int e)
{
    rect[e]          = orgRect[e];
    exporterState[e] = wl_State::Normal;
}
void WindowManager::clearRect(int e) { exporterState[e] = wl_State::Idle; }

bool WindowManager::handleInput(int x, int y)
{
    bool ret = false;

    if (exporterState[0] <= wl_State::Normal && exporterState[1] <= wl_State::Normal)
    {
        for (int i = 0; i < 2; i++)
        {
            if (x > rect[i].x && x < rect[i].x + rect[i].w && y > 0 && y < rect[i].h)
            {
                setFullscreen(i);
                ret = true;
                break;
            }
        }
    }
    else if (exporterState[0] == wl_State::FullScreen)
    {
        if (x > rect[0].x && x < rect[0].x + rect[0].w && y > 0 && y < rect[0].h)
        {
            setNormalSize(0);
            ret = true;
        }
    }
    else if (exporterState[1] == wl_State::FullScreen)
    {
        if (x > rect[1].x && x < rect[1].x + rect[1].w && y > 0 && y < rect[1].h)
        {
            setNormalSize(1);
            ret = true;
        }
    }

    return ret;
}
