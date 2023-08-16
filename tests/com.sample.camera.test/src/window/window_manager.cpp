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

    surface.width  = 1920;
    surface.height = 1080;

    uint32_t exported_type = WL_WEBOS_FOREIGN_WEBOS_EXPORTED_TYPE_VIDEO_OBJECT;
    bool result            = exporter1.initialize(foreign.getDisplay(), foreign.getWebosForeign(),
                                                  surface.wlSurface, exported_type);
    if (result == false)
    {
        std::cout << "exporter1.initialize error" << std::endl;
    }
    else
    {
        std::cout << "exporter1.initialize success" << std::endl;
    }

    result = exporter2.initialize(foreign.getDisplay(), foreign.getWebosForeign(),
                                  surface.wlSurface, exported_type);
    if (result == false)
    {
        std::cout << "exporter2.initialize error" << std::endl;
    }
    else
    {
        std::cout << "exporter2.initialize success" << std::endl;
    }

    foreign.flush();

    std::cout << "exporter1 window ID is : " << exporter1.getWindowID() << std::endl;
    std::cout << "exporter2 window ID is : " << exporter2.getWindowID() << std::endl;

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

        // multi sample
        attr[i++] = EGL_SAMPLE_BUFFERS;
        attr[i++] = 1;
        attr[i++] = EGL_SAMPLES;
        attr[i++] = 4;

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
    exporter1.finalize();
    exporter2.finalize();
    foreign.finalize();

    return true;
}

void WindowManager::setExporterRegion(int exporter_number, int x, int y, int w, int h)
{
    DEBUG_LOG("[%d] %d %d %d %d", exporter_number, x, y, w, h);

    Wayland::Exporter *e         = (exporter_number == 1) ? &exporter1 : &exporter2;
    struct wl_region *region_src = foreign.createRegion(0, 0, 1920, 1080); // don't care
    struct wl_region *region_dst = foreign.createRegion(x, y, w, h);
    e->setRegion(region_src, region_dst);
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
    exporter1.setRegion(region_src, region_dst1);

    // video play
    DEBUG_LOG("Video Play %d %d %d %d", x + w, y, w / 2, h / 2);
    struct wl_region *region_dst2 = foreign.createRegion(x + w, y, w / 2, h / 2);
    exporter2.setRegion(region_src, region_dst2);

    return true;
}