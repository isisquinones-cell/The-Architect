#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>

#include "video_decoder.h"
#include "renderer.h"
#include "input_capture.h"
#include "network_client.h"

#define LOG_TAG "PenStreamNDK"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static penstream::android::VideoDecoder* g_decoder = nullptr;
static penstream::android::Renderer* g_renderer = nullptr;
static penstream::android::NetworkClient* g_client = nullptr;
static penstream::android::InputCapture* g_input = nullptr;
static ANativeWindow* g_window = nullptr;

extern "C" {

JNIEXPORT void JNICALL
Java_com_penstream_app_PenStreamService_nativeInit(JNIEnv* env, jobject thiz) {
    LOGI("Native init");

    g_decoder = new penstream::android::VideoDecoder();
    g_renderer = new penstream::android::Renderer();
    g_client = new penstream::android::NetworkClient();
    g_input = new penstream::android::InputCapture();

    // Initialize renderer
    if (!g_renderer->initialize()) {
        LOGE("Failed to initialize renderer");
    }

    // Setup input callback
    g_input->set_input_callback([](const penstream::android::InputData& data) {
        if (g_client) {
            g_client->send_input(data.x, data.y, data.pressure,
                                data.tilt_x, data.tilt_y, data.buttons);
        }
    });

    // Setup frame callback - decode received data
    g_client->set_frame_callback([](const std::vector<uint8_t>& data) {
        if (g_decoder) {
            // First frame might be a keyframe - decoder will handle it
            g_decoder->decode(data, false);
        }
    });

    LOGI("Native components initialized");
}

JNIEXPORT void JNICALL
Java_com_penstream_app_PenStreamService_nativeStartStreaming(JNIEnv* env, jobject thiz,
                                                              jstring address, jint port) {
    LOGI("Starting streaming");

    const char* addr = env->GetStringUTFChars(address, nullptr);
    int p = static_cast<int>(port);

    if (g_client) {
        if (g_client->connect(addr, p)) {
            g_client->start();
            LOGI("Connected to server at %s:%d", addr, p);
        } else {
            LOGE("Failed to connect to server");
        }
    }

    env->ReleaseStringUTFChars(address, addr);
}

JNIEXPORT void JNICALL
Java_com_penstream_app_PenStreamService_nativeStopStreaming(JNIEnv* env, jobject thiz) {
    LOGI("Stopping streaming");

    if (g_client) {
        g_client->stop();
    }
}

JNIEXPORT void JNICALL
Java_com_penstream_app_PenStreamService_nativeSendInput(JNIEnv* env, jobject thiz,
                                                         jfloat x, jfloat y, jfloat pressure,
                                                         jint tilt_x, jint tilt_y, jint buttons) {
    if (g_input) {
        g_input->process_touch(x, y, pressure);
    }
}

JNIEXPORT void JNICALL
Java_com_penstream_app_PenStreamService_nativeSetSurface(JNIEnv* env, jobject thiz,
                                                          jobject surface) {
    LOGI("Setting surface");

    ANativeWindow* new_window = nullptr;
    if (surface != nullptr) {
        new_window = ANativeWindow_fromSurface(env, surface);
    }

    // Update window reference
    if (g_window != nullptr) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }

    if (new_window != nullptr) {
        g_window = new_window;

        // Set surface on renderer
        if (g_renderer) {
            g_renderer->set_surface(g_window);
            LOGI("Renderer surface set");
        }

        // Set surface on decoder for direct rendering
        if (g_decoder) {
            g_decoder->set_surface(g_window);
            LOGI("Decoder surface set");
        }
    }

    if (new_window != nullptr) {
        ANativeWindow_release(new_window);
    }
}

JNIEXPORT void JNICALL
Java_com_penstream_app_PenStreamService_nativeRelease(JNIEnv* env, jobject thiz) {
    LOGI("Native release");

    if (g_client) {
        g_client->stop();
    }

    delete g_input;
    delete g_decoder;
    delete g_renderer;
    delete g_client;

    g_input = nullptr;
    g_decoder = nullptr;
    g_renderer = nullptr;
    g_client = nullptr;

    if (g_window != nullptr) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

} // extern "C"
