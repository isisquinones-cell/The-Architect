#pragma once

#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

namespace penstream::android {

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize();
    void set_surface(ANativeWindow* window);
    void render_frame(const void* y_data, const void* uv_data,
                      int32_t width, int32_t height);
    void release();

private:
    bool create_context();
    void setup_shaders();

    ANativeWindow* m_window;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
    EGLConfig  m_config;
    GLuint m_program;
    GLuint m_texture;
    bool m_initialized;
};

} // namespace penstream::android
