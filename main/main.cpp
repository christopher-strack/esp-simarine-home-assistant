#include "config.hpp"
#include "esp_system.h"
#include "mqtt_client.hpp"
#include "mqtt_logger.hpp"
#include "wifi_connector.hpp"
#include "wifi_utils.hpp"

#include "spymarine/buffer.hpp"
#include "spymarine/device_ostream.hpp"
#include "spymarine/discover.hpp"
#include "spymarine/home_assistant.hpp"
#include "spymarine/read_devices.hpp"
#include "spymarine/sensor_reader.hpp"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include <chrono>

namespace {
constexpr auto TAG = "spymarine";

void send_home_assistant_device_discovery(
    const std::vector<spymarine::device>& devices, mqtt_client& client) {
  ESP_LOGI(TAG, "Sending Home Assistant device discovery messages");

  for (const auto& device : devices) {
    const auto message =
        spymarine::make_home_assistant_device_discovery_message(device);
    const auto published = client.publish(
        message.topic.c_str(), message.payload, mqtt_qos::at_least_once, false);
    if (!published) {
      ESP_LOGE(TAG, "Couldn't send device discovery message");
    }
  }
}

void publish_sensor_values(const std::vector<spymarine::device>& devices,
                           mqtt_client& client) {
  ESP_LOGI(TAG, "Sending Home Assistant sensor messags");

  for (const auto& device : devices) {
    const auto message = make_home_assistant_state_message(device);
    client.publish(message.topic.c_str(), message.payload,
                   mqtt_qos::at_most_once, false);
  }
}

void process_sensor_values(
    const std::vector<spymarine::device>& devices,
    spymarine::moving_average_sensor_reader<spymarine::udp_socket>&
        sensor_reader,
    mqtt_client& client) {
  ESP_LOGI(TAG, "Start processing sensor values");

  std::atomic<bool> reinitialize = false;
  client.subscribe("homeassistant/status", mqtt_qos::at_least_once,
                   [&](std::string_view data) {
                     if (data == "online") {
                       reinitialize = true;
                     }
                   });

  while (true) {
    if (reinitialize) {
      send_mqtt_logger_device_discovery();
      send_home_assistant_device_discovery(devices, client);
      sensor_reader.read_and_update().transform(
          [&](bool) { publish_sensor_values(devices, client); });
      reinitialize = false;
    }

    const auto result =
        sensor_reader.read_and_update().transform([&](bool window_completed) {
          if (window_completed) {
            publish_sensor_values(devices, client);
          }
        });

    if (!result) {
      ESP_LOGE(TAG, "Failed to read sensor values: %s",
               spymarine::error_message(result.error()).c_str());
    }
  }
}

bool start(mqtt_client& client) {
  spymarine::buffer buffer;

  auto devices = spymarine::discover().and_then([&](const auto ip) {
    ESP_LOGI(TAG, "Read devices");

    return spymarine::read_devices<spymarine::tcp_socket>(
        buffer, ip, spymarine::simarine_default_tcp_port, make_device_filter());
  });

  if (!devices) {
    ESP_LOGE(TAG, "Failed to read devices: %s",
             spymarine::error_message(devices.error()).c_str());
    return false;
  }

  if (devices->size() == 0) {
    ESP_LOGE(TAG, "No devices found");
    return false;
  }

  ESP_LOGI(TAG, "Found %d devices", devices->size());

  auto sensor_reader = spymarine::make_moving_average_sensor_reader(
      buffer, sensor_update_interval, *devices);

  if (!sensor_reader) {
    ESP_LOGE(TAG, "Failed to make sensor reader: %s",
             spymarine::error_message(sensor_reader.error()).c_str());
    return false;
  }

  process_sensor_values(*devices, *sensor_reader, client);
  return true;
}

} // namespace

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_connector connector{wifi_ssid, wifi_password};
  {
    auto wifi_connected_promise = connector.make_connected_promise();
    connector.start();
    wifi_connected_promise.wait();
  }

  mqtt_client mqtt_client{make_mqtt_config()};
  {
    auto mqtt_client_connected_promise = mqtt_client.make_connected_promise();
    mqtt_client.start();
    mqtt_client_connected_promise.wait();
  }

  setup_mqtt_logger(mqtt_client);
  send_mqtt_logger_device_discovery();

  if (!start(mqtt_client)) {
    ESP_LOGE(TAG, "Failed to initialize, restarting in a moment...");
    std::this_thread::sleep_for(wifi_retry_interval);
    esp_restart();
  }
}
