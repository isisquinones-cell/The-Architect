#include "renderer.h"
#include <android/log.h>

#define LOG_TAG "PenStreamRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace penstream::android {

Renderer::Renderer()
    : m_window(nullptr)
    , m_display(EGL_NO_DISPLAY)
    , m_surface(EGL_NO_SURFACE)
    , m_context(EGL_NO_CONTEXT)
    , m_config(nullptr)
    , m_program(0)
    , m_texture(0)
    , m_initialized(false)
{}

Renderer::~Renderer() {
    release();
}

bool Renderer::initialize() {
    if (m_display == EGL_NO_DISPLAY) {
        m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (m_display == EGL_NO_DISPLAY) {
            LOGE("Failed to get EGL display");
            return false;
        }

        if (!eglInitialize(m_display, nullptr, nullptr)) {
            LOGE("Failed to initialize EGL");
            return false;
        }

        // Elegir configuración EGL
        const EGLint config_attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_NONE
        };

        EGLint num_configs = 0;
        if (!eglChooseConfig(m_display, config_attribs, &m_config, 1, &num_configs)
            || num_configs == 0) {
            LOGE("Failed to choose EGL config");
            return false;
        }
    }

    m_initialized = true;
    LOGI("Renderer initialized");
    return true;
}

void Renderer::set_surface(ANativeWindow* window) {
    if (m_window == window) {
        return;
    }

    // Limpiar superficie anterior
    if (m_surface != EGL_NO_SURFACE) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(m_display, m_surface);
        m_surface = EGL_NO_SURFACE;
    }

    m_window = window;

    if (m_window != nullptr && m_config != nullptr) {
        const EGLint surface_attribs[] = {
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE
        };
        m_surface = eglCreateWindowSurface(m_display, m_config, m_window, surface_attribs);

        if (m_surface == EGL_NO_SURFACE) {
            LOGE("Failed to create EGL surface");
            return;
        }

        if (m_context == EGL_NO_CONTEXT) {
            const EGLint context_attribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 3,
                EGL_NONE
            };
            m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, context_attribs);
        }

        eglMakeCurrent(m_display, m_surface, m_surface, m_context);
        setup_shaders();
    }
}

void Renderer::setup_shaders() {
    // Limpiar a color de fondo por ahora
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void Renderer::render_frame(const void* /*y_data*/, const void* /*uv_data*/,
                            int32_t /*width*/, int32_t /*height*/) {
    if (!m_initialized || m_surface == EGL_NO_SURFACE) {
        return;
    }

    eglMakeCurrent(m_display, m_surface, m_surface, m_context);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(m_display, m_surface);
}

void Renderer::release() {
    if (m_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    if (m_context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_display, m_context);
        m_context = EGL_NO_CONTEXT;
    }

    if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(m_display, m_surface);
        m_surface = EGL_NO_SURFACE;
    }

    if (m_display != EGL_NO_DISPLAY) {
        eglTerminate(m_display);
        m_display = EGL_NO_DISPLAY;
    }

    m_window = nullptr;
    m_config = nullptr;
    m_initialized = false;
}

} // namespace penstream::android
