#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

namespace penstream::android {

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    void set_frame_callback(std::function<void(const std::vector<uint8_t>&)> callback);

    bool connect(const std::string& address, int port);
    void start();
    void stop();

    void send_input(float x, float y, float pressure,
                    int8_t tilt_x, int8_t tilt_y, uint8_t buttons);

private:
    void receive_loop();

    std::string m_server_address;
    int m_server_port;
    std::atomic<bool> m_running;
    std::thread m_receive_thread;

    std::function<void(const std::vector<uint8_t>&)> m_frame_callback;

    int m_socket;
    uint32_t m_sequence_num;
};

} // namespace penstream::android
