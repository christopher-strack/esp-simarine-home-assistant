#pragma once

#include "esp_event_base.h"
#include "esp_netif_types.h"
#include "esp_timer.h"
#include "esp_wifi_types_generic.h"

#include <atomic>
#include <chrono>
#include <future>
#include <string_view>

class wifi_connected_promise {
public:
  wifi_connected_promise();
  ~wifi_connected_promise();

  void wait();

private:
  static void event_handler(void* arg, esp_event_base_t eventBase,
                            int32_t eventId, void* eventData);

  std::promise<void> _promise;
  std::future<void> _future;
  esp_event_handler_instance_t _event_handler_instance;
  bool _value_set{false};
};

/*! A simple Wifi connector that tries to establish a connection to the given
 * Wifi. Continuously attempts to reconnect at intervals defined by
 * retry_interval if the WiFi network disconnects, regardless of the
 * disconnection reason.
 */
class wifi_connector {
public:
  static constexpr std::chrono::seconds retry_interval{10};

  wifi_connector(std::string_view ssid, std::string_view password);
  explicit wifi_connector(wifi_sta_config_t wifi_config);
  wifi_connector(const wifi_connector& other) = delete;

  ~wifi_connector();

  wifi_connected_promise make_connected_promise();

  void start();

  wifi_connector& operator=(const wifi_connector& other) = delete;

private:
  void on_station_disconnected(wifi_err_reason_t reason);

  void create_reconnect_timer();
  void delete_reconnect_timer();

  static void event_handler(void* arg, esp_event_base_t eventBase,
                            int32_t eventId, void* eventData);

  static void reconnect_timer_handler(void* arg);

  esp_netif_t* _esp_netif{nullptr};
  esp_event_handler_instance_t _instance_any_id{nullptr};
  esp_timer_handle_t _reconnect_timer_handle;
  std::atomic<bool> _stopping{false};
};
