#include "config_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace penstream::config {

using json = nlohmann::json;

Config load_config() {
    Config config;

    std::string config_path = "config.json";

    // Try to load from file
    if (std::filesystem::exists(config_path)) {
        try {
            std::ifstream file(config_path);
            json j = json::parse(file);

            config.port = j.value("port", config.port);
            config.width = j.value("width", config.width);
            config.height = j.value("height", config.height);
            config.fps = j.value("fps", config.fps);
            config.bitrate_kbps = j.value("bitrate_kbps", config.bitrate_kbps);
            config.encoder = j.value("encoder", config.encoder);
            config.low_latency = j.value("low_latency", config.low_latency);
            config.require_pin = j.value("require_pin", config.require_pin);
            config.pin = j.value("pin", config.pin);
            config.fec_strength = j.value("fec_strength", config.fec_strength);
            config.enable_stats = j.value("enable_stats", config.enable_stats);

            spdlog::info("Loaded config from {}", config_path);
        } catch (const std::exception& e) {
            spdlog::warn("Failed to load config: {}. Using defaults.", e.what());
        }
    } else {
        // Create default config file
        save_config(config);
        spdlog::info("Created default config file");
    }

    // Override with environment variables
    if (const char* env_port = std::getenv("PENSTREAM_PORT")) {
        config.port = static_cast<uint16_t>(std::stoi(env_port));
    }
    if (const char* env_bitrate = std::getenv("PENSTREAM_BITRATE")) {
        config.bitrate_kbps = static_cast<uint32_t>(std::stoi(env_bitrate));
    }
    if (const char* env_fps = std::getenv("PENSTREAM_FPS")) {
        config.fps = static_cast<uint32_t>(std::stoi(env_fps));
    }

    return config;
}

bool save_config(const Config& config) {
    try {
        json j;
        j["port"] = config.port;
        j["width"] = config.width;
        j["height"] = config.height;
        j["fps"] = config.fps;
        j["bitrate_kbps"] = config.bitrate_kbps;
        j["encoder"] = config.encoder;
        j["low_latency"] = config.low_latency;
        j["require_pin"] = config.require_pin;
        j["pin"] = config.pin;
        j["fec_strength"] = config.fec_strength;
        j["enable_stats"] = config.enable_stats;

        std::ofstream file("config.json");
        file << j.dump(4);

        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save config: {}", e.what());
        return false;
    }
}

} // namespace penstream::config
