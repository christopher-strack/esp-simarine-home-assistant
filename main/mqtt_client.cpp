#include "mqtt_client.hpp"

#include "esp_log.h"
#include "esp_wifi.h"

namespace {

static const char* TAG = "mqtt_client";

} // namespace

esp_mqtt_client_config_t
create_mqtt_user_config(const char* broker_uri, const char* root_ca_certificate,
                        const char* username, const char* password) {
  esp_mqtt_client_config_t config{};
  config.broker.address.uri = broker_uri;
  config.broker.verification.certificate = root_ca_certificate;
  config.credentials.username = username;
  config.credentials.authentication.password = password;
  return config;
}

mqtt_client::mqtt_client(esp_mqtt_client_config_t config)
    : _config{std::move(config)}, _client{nullptr} {
  _client = esp_mqtt_client_init(&config);

  ESP_ERROR_CHECK(esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY,
                                                 mqtt_event_handler, this));
}

mqtt_client::~mqtt_client() {
  ESP_ERROR_CHECK(esp_mqtt_client_unregister_event(_client, MQTT_EVENT_ANY,
                                                   mqtt_event_handler));

  if (_started) {
    ESP_ERROR_CHECK(esp_mqtt_client_stop(_client));
  }
}

void mqtt_client::start() {
  ESP_ERROR_CHECK(esp_mqtt_client_start(_client));
  _started = true;
}

namespace {

void report_error(esp_mqtt_error_codes_t error_handle) {
  if (error_handle.error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
    ESP_LOGE(TAG, "Transport error");
  } else if (error_handle.error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
    ESP_LOGE(TAG, "Connection refused");
  }
}

} // namespace

void mqtt_client::mqtt_event_handler(void* arg, esp_event_base_t eventBase,
                                     int32_t event_id, void* event_data) {
  switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "Connected");
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "Disconnected");
    break;
  case MQTT_EVENT_ERROR: {
    const auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    report_error(*event->error_handle);
    break;
  }
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "Subscribed");
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "Unsubscribed");
    break;
  case MQTT_EVENT_DATA: {
    auto _this = static_cast<mqtt_client*>(arg);
    const auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    _this->notify_data(std::string_view{event->topic, size_t(event->topic_len)},
                       std::string_view{event->data, size_t(event->data_len)});
    break;
  }
  default:
    break;
  }
}

bool mqtt_client::publish(const char* topic, const std::string_view data,
                          mqtt_qos qos, const bool retain) {
  const auto result = esp_mqtt_client_publish(
      _client, topic, data.data(), data.size(), static_cast<int>(qos), retain);
  return result >= 0;
}

bool mqtt_client::subscribe(const char* topic, mqtt_qos qos,
                            subscribe_callback callback) {
  const auto result =
      esp_mqtt_client_subscribe_single(_client, topic, static_cast<int>(qos));
  if (result >= 0) {
    std::unique_lock lock{_subscribe_map_mutex};
    _subscribe_map.emplace(std::string{topic}, std::move(callback));
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to subscribe %i", result);
  }
  return false;
}

mqtt_connected_promise mqtt_client::make_connected_promise() {
  return mqtt_connected_promise{_client};
}

void mqtt_client::notify_data(std::string_view topic, std::string_view data) {
  std::unique_lock lock{_subscribe_map_mutex};
  auto it = _subscribe_map.find(topic);
  if (it != _subscribe_map.end()) {
    it->second(data);
  }
}

mqtt_connected_promise::mqtt_connected_promise(esp_mqtt_client_handle_t client)
    : _client{client}, _future{_promise.get_future()} {
  ESP_ERROR_CHECK(esp_mqtt_client_register_event(_client, MQTT_EVENT_CONNECTED,
                                                 mqtt_event_handler, this));
}

mqtt_connected_promise::~mqtt_connected_promise() {
  ESP_ERROR_CHECK(esp_mqtt_client_unregister_event(_client, MQTT_EVENT_ANY,
                                                   mqtt_event_handler));
}

void mqtt_connected_promise::wait() { _future.wait(); }

void mqtt_connected_promise::mqtt_event_handler(void* arg,
                                                esp_event_base_t eventBase,
                                                int32_t event_id,
                                                void* event_data) {
  auto this_ = static_cast<mqtt_connected_promise*>(arg);
  if (!this_->_value_set) {
    this_->_promise.set_value();
    this_->_value_set = true;
  }
}
