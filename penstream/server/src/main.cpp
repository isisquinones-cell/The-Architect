#include <winsock2.h>   // debe ir ANTES de windows.h
#include <windows.h>
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#include "config/config_loader.h"
#include "capture/dxgi_capturer.h"
#include "encode/nvenc_encoder.h"
#include "encode/encoder_interface.h"
#include "network/udp_transport.h"
#include "input/input_handler.h"

namespace penstream {

static std::atomic<bool> g_running{true};

void signal_handler(int /*signal*/) {
    g_running.store(false);
}

int run() {
    auto config = config::load_config();

    std::cout << "========================================" << std::endl;
    std::cout << "  PenStream Server v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Port: " << config.port << std::endl;
    std::cout << "Target: " << config.width << "x" << config.height << "@" << config.fps << "fps" << std::endl;
    std::cout << "Bitrate: " << config.bitrate_kbps << " kbps" << std::endl;
    std::cout << "Encoder: " << config.encoder << std::endl;
    std::cout << "========================================" << std::endl;

    // Inicializar captura DXGI
    capture::DXGICapturer capturer;
    std::cout << "[1/4] Initializing screen capture..." << std::flush;
    if (!capturer.initialize()) {
        std::cerr << " FAILED" << std::endl;
        std::cerr << "Error: Failed to initialize DXGI capturer" << std::endl;
        std::cerr << "Make sure you have a DirectX 11 compatible GPU." << std::endl;
        return 1;
    }
    std::cout << " OK (" << capturer.get_width() << "x" << capturer.get_height() << ")" << std::endl;

    // Inicializar encoder
    encode::NVEncoder encoder;
    encoder.set_d3d_device(capturer.get_device());

    encode::EncodeConfig encode_config;
    encode_config.width      = config.width;
    encode_config.height     = config.height;
    encode_config.bitrate_bps = config.bitrate_kbps * 1000;
    encode_config.fps        = config.fps;
    encode_config.low_latency = config.low_latency;

    std::cout << "[2/4] Initializing NVENC encoder..." << std::flush;
    if (!encoder.initialize(encode_config)) {
        std::cerr << " FAILED" << std::endl;
        std::cerr << "Error: Failed to initialize NVENC encoder" << std::endl;
        std::cerr << "Make sure you have an NVIDIA GPU and drivers installed." << std::endl;
        std::cerr << "Fallback to AMF (AMD) or QSV (Intel) not yet implemented." << std::endl;
        return 1;
    }
    std::cout << " OK" << std::endl;

    // Inicializar red UDP
    network::UDPTransport transport(static_cast<uint16_t>(config.port));
    std::cout << "[3/4] Initializing UDP transport..." << std::flush;
    if (!transport.initialize()) {
        std::cerr << " FAILED" << std::endl;
        std::cerr << "Error: Failed to initialize UDP transport on port " << config.port << std::endl;
        std::cerr << "Make sure no other application is using this port." << std::endl;
        return 1;
    }
    std::cout << " OK" << std::endl;

    // Inicializar input handler
    input::InputHandler input_handler;
    std::cout << "[4/4] Initializing input handler..." << std::flush;
    if (!input_handler.initialize()) {
        std::cerr << " FAILED" << std::endl;
        std::cerr << "Error: Failed to initialize input handler" << std::endl;
        return 1;
    }
    std::cout << " OK" << std::endl;

    std::cout << "========================================" << std::endl;
    std::cout << "Server started. Waiting for client..." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    std::cout << "========================================" << std::endl;

    uint32_t frame_id = 0;
    auto frame_interval   = std::chrono::microseconds(1000000 / config.fps);
    auto last_frame_time  = std::chrono::steady_clock::now();
    uint32_t frames_sent  = 0;
    auto stats_time       = std::chrono::steady_clock::now();

    while (g_running.load()) {
        if (!transport.has_client()) {
            std::vector<uint8_t> recv_buffer;
            sockaddr_in client_addr{};

            if (transport.receive(recv_buffer, client_addr, 100)) {
                if (recv_buffer.size() >= 12) {
                    if (recv_buffer[0] == 0x50 && recv_buffer[1] == 0x53) {
                        uint8_t type = recv_buffer[2];
                        if (type == 0x10) { // CONNECT_REQ
                            std::cout << "Client connection request received" << std::endl;
                            transport.send_connect_response(
                                client_addr, true,
                                static_cast<uint16_t>(config.width),
                                static_cast<uint16_t>(config.height),
                                static_cast<uint16_t>(config.bitrate_kbps));
                            std::cout << "Client connected!" << std::endl;
                        }
                    }
                }
            }
        }

        if (!transport.has_client()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto now     = std::chrono::steady_clock::now();
        auto elapsed = now - last_frame_time;

        if (elapsed >= frame_interval) {
            capture::Frame frame;
            if (capturer.capture_frame(frame)) {
                std::vector<uint8_t> encoded_data;
                if (encoder.encode(frame, encoded_data) && !encoded_data.empty()) {
                    if (transport.send_video_frame(encoded_data, frame_id, frame.timestamp_us,
                                                   frame.width, frame.height)) {
                        frames_sent++;
                    }
                }
            }
            frame_id++;
            last_frame_time = now;
        }

        input_handler.process_pending_inputs();

        {
            std::vector<uint8_t> recv_buffer;
            sockaddr_in dummy_addr{};
            if (transport.receive(recv_buffer, dummy_addr, 0)) {
                if (recv_buffer.size() >= sizeof(network::InputPacket)) {
                    const auto* input =
                        reinterpret_cast<const network::InputPacket*>(recv_buffer.data());

                    if (input->header.type == 0x02) { // TOUCH_INPUT
                        input::InputEvent event;
                        event.x         = input->x;
                        event.y         = input->y;
                        event.pressure  = input->pressure;
                        event.tilt_x    = input->tilt_x;
                        event.tilt_y    = input->tilt_y;
                        event.buttons   = input->buttons;
                        event.timestamp = input->header.timestamp;
                        input_handler.queue_input(event);
                    }
                }
            }
        }

        // Stats cada 5 segundos
        auto stats_elapsed = now - stats_time;
        if (std::chrono::duration_cast<std::chrono::seconds>(stats_elapsed).count() >= 5) {
            auto fps   = static_cast<float>(frames_sent) /
                         static_cast<float>(
                             std::chrono::duration_cast<std::chrono::seconds>(stats_elapsed).count());
            auto stats = transport.get_stats();
            std::cout << "Stats: " << fps << " fps | "
                      << stats.bytes_sent / 1024 << " KB sent | "
                      << stats.packets_received << " inputs received" << std::endl;
            frames_sent = 0;
            stats_time  = std::chrono::steady_clock::now();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "Shutting down..." << std::endl;
    return 0;
}

} // namespace penstream

int main(int /*argc*/, char* /*argv*/[]) {
    std::signal(SIGINT,  penstream::signal_handler);
    std::signal(SIGTERM, penstream::signal_handler);
    SetConsoleOutputCP(CP_UTF8);
    return penstream::run();
}
