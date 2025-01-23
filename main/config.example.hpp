#pragma once

#include "mqtt_client.hpp"

#include "spymarine/read_devices.hpp"

#include <chrono>

constexpr auto wifi_retry_interval = std::chrono::seconds{5};
constexpr auto sensor_update_interval = std::chrono::minutes{1};

constexpr auto wifi_ssid = "SSID";
constexpr auto wifi_password = "password";

constexpr auto mqtt_broker_uri = "mqtts://unique-id.s1.eu.hivemq.cloud:8883";
constexpr auto mqtt_username = "user";
constexpr auto mqtt_password = "password";

constexpr auto mqtt_root_ca_certificate = R"(
-----BEGIN CERTIFICATE-----
CERTIFICATE DATA
-----END CERTIFICATE-----
)";

inline esp_mqtt_client_config_t make_mqtt_config() {
  esp_mqtt_client_config_t config{};
  config.broker.address.uri = mqtt_broker_uri;
  config.broker.verification.certificate = mqtt_root_ca_certificate;
  config.credentials.username = mqtt_username;
  config.credentials.authentication.password = mqtt_password;
  return config;
}

inline auto make_device_filter() {
  return spymarine::filter_by_device_type<spymarine::temperature_device,
                                          spymarine::tank_device,
                                          spymarine::battery_device>{};
}
