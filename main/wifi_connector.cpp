#include "wifi_connector.hpp"
#include "wifi_utils.hpp"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"

namespace {

const char* TAG = "wifi_connector";

template <size_t N>
void copy_string(uint8_t (&target)[N], const std::string_view str) {
  if (N < str.size() + 1) {
    ESP_LOGE(TAG, "string is too long");
    abort();
  }

  std::copy_n(str.begin(), str.size(), target);
  target[str.size()] = '\0';
}

wifi_sta_config_t create_wifi_sta_config(const std::string_view ssid,
                                         const std::string_view password) {
  wifi_sta_config_t config{};
  copy_string(config.ssid, ssid);
  copy_string(config.password, password);

  config.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  config.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
  copy_string(config.sae_h2e_identifier, "");

  return config;
}

void connect_wifi() {
  const auto err = esp_wifi_connect();

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(err));
  }
}

} // namespace

namespace {} // namespace

wifi_connected_promise::wifi_connected_promise()
    : _future{_promise.get_future()} {
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, this,
      &_event_handler_instance));
}

wifi_connected_promise::~wifi_connected_promise() {
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &_event_handler_instance));
}

void wifi_connected_promise::wait() { _future.wait(); }

void wifi_connected_promise::event_handler(void* arg,
                                           esp_event_base_t eventBase,
                                           int32_t eventId, void* eventData) {

  if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
    auto this_ = static_cast<wifi_connected_promise*>(arg);
    if (!this_->_value_set) {
      this_->_promise.set_value();
      this_->_value_set = true;
    }
  }
}

wifi_connector::wifi_connector(std::string_view ssid, std::string_view password)
    : wifi_connector{create_wifi_sta_config(ssid, password)} {}

wifi_connector::wifi_connector(wifi_sta_config_t wifi_sta_config)
    : _esp_netif{esp_netif_create_default_wifi_sta()} {
  ESP_LOGI(TAG, "Starting wifi...");

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, this, &_instance_any_id));

  create_reconnect_timer();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config{
      .sta = std::move(wifi_sta_config),
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

wifi_connector::~wifi_connector() {
  ESP_LOGI(TAG, "Stopping wifi...");
  _stopping = true;

  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());

  delete_reconnect_timer();

  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &_instance_any_id));

  esp_netif_destroy_default_wifi(_esp_netif);
  ESP_LOGI(TAG, "Wifi stopped");
}

wifi_connected_promise wifi_connector::make_connected_promise() {
  return wifi_connected_promise{};
}

void wifi_connector::start() {
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Wifi started");
}

void wifi_connector::on_station_disconnected(const wifi_err_reason_t reason) {
  // If we are in the process of stopping the connector we don't want to try
  // and reconnect. The handler is called even if we unregistering it before
  // calling esp_wifi_stop().
  if (_stopping) {
    return;
  }

  ESP_LOGI(TAG, "Wifi disconnected: %s", wifi_disconnect_reason_string(reason));
  ESP_LOGI(TAG, "Attempting to reconnect in %lld seconds",
           retry_interval.count());

  const auto timeout_us =
      std::chrono::duration_cast<std::chrono::microseconds>(retry_interval);
  const auto err =
      esp_timer_start_once(_reconnect_timer_handle, timeout_us.count());

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed start reconnect timer: %s", esp_err_to_name(err));
  }
}

void wifi_connector::create_reconnect_timer() {
  const auto timer_args = esp_timer_create_args_t{
      .callback = &reconnect_timer_handler,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "reconnect_timer",
      .skip_unhandled_events = false,
  };
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &_reconnect_timer_handle));
}

void wifi_connector::delete_reconnect_timer() {
  if (esp_timer_is_active(_reconnect_timer_handle)) {
    ESP_ERROR_CHECK(esp_timer_stop(_reconnect_timer_handle));
  }
  ESP_ERROR_CHECK(esp_timer_delete(_reconnect_timer_handle));
}

void wifi_connector::event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data) {
  auto this_ = static_cast<wifi_connector*>(arg);

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    ESP_LOGI(TAG, "Connecting to wifi...");
    connect_wifi();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    const auto disconnected_event =
        static_cast<wifi_event_sta_disconnected_t*>(event_data);
    const auto reason =
        static_cast<wifi_err_reason_t>(disconnected_event->reason);
    this_->on_station_disconnected(reason);
  }
}

void wifi_connector::reconnect_timer_handler(void*) {
  ESP_LOGI(TAG, "Retry connecting wifi...");
  connect_wifi();
}
