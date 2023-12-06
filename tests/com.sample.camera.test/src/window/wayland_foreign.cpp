// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// SPDX-License-Identifier: Apache-2.0

#include "wayland_foreign.h"
#include "appParm.h"
#include <cstdio>
#include <cstring>

int mSx, mSy, mState;

namespace Wayland
{

static void pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                                 struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
    // fprintf(stdout, "x => %d y=> %d\n", sx, sy);
    // fprintf(stdout, "x => %d y=> %d\n", sx, sy);
}

static void pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
                                 struct wl_surface *surface)
{
    fprintf(stdout, "\n");
}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t time,
                                  wl_fixed_t sx, wl_fixed_t sy)
{
    // fprintf(stdout, "x => %d, y => %d \n", sx>>8, sy>>8);
    mSx = sx >> 8;
    mSy = sy >> 8;
}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
                                  uint32_t time, uint32_t button, uint32_t state)
{
    // fprintf(stdout, "state => %d \n", state);
    mState = state;

    if (state == 0)
    {
        // printf("[%s] x => %d, y => %d\n", __func__, mSx, mSy);
        miCameraApp->handleInput(mSx, mSy);
    }
}

static void pointer_handle_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
                                uint32_t axis, wl_fixed_t value)
{
    fprintf(stdout, "pointer_handle_axis\n");
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,  pointer_handle_leave, pointer_handle_motion,
    pointer_handle_button, pointer_handle_axis,
};

static void seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t caps)
{
    // fprintf(stderr, "[%s]: seat => %p\n", __FUNCTION__, seat);

    static struct wl_pointer *_g_pst_pointer = NULL;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !_g_pst_pointer)
    {
        _g_pst_pointer = wl_seat_get_pointer(seat);
        // fprintf(stderr, "pointer => %p\n", _g_pst_pointer);

        wl_pointer_add_listener(_g_pst_pointer, &pointer_listener, NULL);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && _g_pst_pointer)
    {
        fprintf(stderr, "destroy pointer\n");
        wl_pointer_destroy(_g_pst_pointer);
        _g_pst_pointer = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

static void display_handle_global(void *waylandData, struct wl_registry *registry, uint32_t id,
                                  const char *interface, uint32_t version)
{
    // fprintf(stdout, "%s, id : %d  \n", interface, id);

    auto foreign = (Foreign *)waylandData;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        foreign->setCompositor(
            (struct wl_compositor *)wl_registry_bind(registry, id, &wl_compositor_interface, 1));
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        foreign->setShell(
            (struct wl_shell *)wl_registry_bind(registry, id, &wl_shell_interface, 1));
    }
    else if (strcmp(interface, "wl_webos_shell") == 0)
    {
        foreign->setWebosShell(
            (struct wl_webos_shell *)wl_registry_bind(registry, id, &wl_webos_shell_interface, 1));
    }
    else if (strcmp(interface, "wl_webos_foreign") == 0)
    {
        foreign->setWebosForeign((struct wl_webos_foreign *)wl_registry_bind(
            registry, id, &wl_webos_foreign_interface, 1));
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        foreign->setSeat((struct wl_seat *)wl_registry_bind(registry, id, &wl_seat_interface, 1));
        wl_seat_add_listener(foreign->getSeat(), &seat_listener, NULL);
    }
}

static const struct wl_registry_listener registryListener = {display_handle_global};

Foreign::Foreign(void)
    : display(nullptr), registry(nullptr), compositor(nullptr), shell(nullptr), webosShell(nullptr),
      webosForeign(nullptr)
{
}

Foreign::~Foreign(void) {}

bool Foreign::initialize(void)
{
    display = wl_display_connect(nullptr);
    if (!display)
    {
        return false;
    }

    registry = wl_display_get_registry(display);
    if (!registry)
    {
        return false;
    }

    wl_registry_add_listener(registry, &registryListener, this);

    wl_display_dispatch(display);

    return true;
}

void Foreign::finalize(void)
{
    if (webosForeign)
        wl_webos_foreign_destroy(webosForeign);
    if (webosShell)
        wl_webos_shell_destroy(webosShell);
    if (shell)
        wl_shell_destroy(shell);
    if (compositor)
        wl_compositor_destroy(compositor);

    if (display)
    {
        wl_display_flush(display);
        wl_display_disconnect(display);
    }
}

void Foreign::setCompositor(struct wl_compositor *compositor) { this->compositor = compositor; }

void Foreign::setShell(struct wl_shell *shell) { this->shell = shell; }

void Foreign::setWebosShell(struct wl_webos_shell *webosShell) { this->webosShell = webosShell; }

void Foreign::setWebosForeign(struct wl_webos_foreign *webosForeign)
{
    this->webosForeign = webosForeign;
}

void Foreign::setSeat(struct wl_seat *seat) { this->seat = seat; }

struct wl_display *Foreign::getDisplay(void) { return display; }

struct wl_compositor *Foreign::getCompositor(void) { return compositor; }

struct wl_shell *Foreign::getShell(void) { return shell; }

struct wl_webos_shell *Foreign::getWebosShell(void) { return webosShell; }

struct wl_webos_foreign *Foreign::getWebosForeign(void) { return webosForeign; }

struct wl_seat *Foreign::getSeat(void) { return seat; }

struct wl_region *Foreign::createRegion(int32_t x, int32_t y, int32_t width, int32_t height)
{
    struct wl_region *region = wl_compositor_create_region(compositor);
    wl_region_add(region, x, y, width, height);

    return region;
}

void Foreign::destroyRegion(struct wl_region *region) { wl_region_destroy(region); }

bool Foreign::flush(void) { return wl_display_flush(display); }

} // namespace Wayland
