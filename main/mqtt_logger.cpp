#include "mqtt_logger.hpp"

#include "esp_log.h"

#include <optional>

namespace {

struct mqtt_logger {
  explicit mqtt_logger(mqtt_client& client) : _client{client} {}

  int log(const char* str, va_list list) {
    const auto result = vsprintf(_buffer.data(), str, list);
    if (result > 0) {
      const auto data = std::string_view{_buffer.data(), size_t(result)};
      if (const auto pos = data.find(')'); pos + 2 < data.size()) {
        _client.publish("simarine_esp/log", data.substr(pos + 2),
                        mqtt_qos::at_least_once, false);
      }
    }
    return vprintf(str, list);
  }

  void send_device_discovery() {
    const std::string_view discovery_message = R"(
{
"dev":{"ids": "simarine_esp_log", "name": "Simarine ESP Log"},
"o":{"name": "simarine_esp_log","sw": "0.1","url": "https://github.com/christopher-strack/esp_simarine_home_assistant"},
"cmps": {"simarine_esp_log": {"p": "sensor","unique_id": "simarine_esp_log"}},
"state_topic": "simarine_esp/log","qos": 1
}
)";

    _client.publish("homeassistant/device/esp_log/config", discovery_message,
                    mqtt_qos::at_least_once, false);
  }

private:
  mqtt_client& _client;
  std::array<char, 256> _buffer;
};

std::optional<mqtt_logger> g_logger;

int mqtt_log(const char* str, va_list list) { return g_logger->log(str, list); }
} // namespace

void setup_mqtt_logger(mqtt_client& client) {
  g_logger.emplace(client);
  esp_log_set_vprintf(mqtt_log);
}

void send_mqtt_logger_device_discovery() { g_logger->send_device_discovery(); }
