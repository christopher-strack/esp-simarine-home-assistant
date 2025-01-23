#pragma once

#include "string_hash.hpp"

#include "mqtt_client.h"

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

/*! The quality of service to use for publishing messages
 */
enum class mqtt_qos {
  at_most_once = 0,
  at_least_once = 1,
  exactly_once = 2,
};

using subscribe_callback = std::function<void(std::string_view)>;

class mqtt_connected_promise {
public:
  mqtt_connected_promise(esp_mqtt_client_handle_t client);
  ~mqtt_connected_promise();

  void wait();

private:
  static void mqtt_event_handler(void* arg, esp_event_base_t eventBase,
                                 int32_t event_id, void* event_data);

  esp_mqtt_client_handle_t _client;
  std::promise<void> _promise;
  std::future<void> _future;
  esp_event_handler_instance_t _event_handler_instance;
  bool _value_set{false};
};

/*! Simple wrapper around esp-idf's MQTT client implementation
 */
class mqtt_client {
public:
  explicit mqtt_client(esp_mqtt_client_config_t config);
  mqtt_client(const mqtt_client& other) = delete;

  ~mqtt_client();

  mqtt_client& operator=(const mqtt_client& other) = delete;

  void start();

  /*! Send a message to the broker for the given topic. Blocks
   * until the message has been sent and a response is received (
   * depending on the quality of service).
   *
   * Returns true if the message was published successfully or
   * false otherwise. The result depends on the quaility of service.
   */
  bool publish(const char* topic, std::string_view data, mqtt_qos qos,
               bool retain);

  bool subscribe(const char* topic, mqtt_qos qos, subscribe_callback callback);

  mqtt_connected_promise make_connected_promise();

private:
  void notify_data(std::string_view topic, std::string_view data);

  static void mqtt_event_handler(void* arg, esp_event_base_t eventBase,
                                 int32_t event_id, void* event_data);

  esp_mqtt_client_config_t _config;
  esp_mqtt_client_handle_t _client;
  subscribe_callback _callback;

  std::atomic<bool> _started = false;

  std::mutex _subscribe_map_mutex;
  std::unordered_map<std::string, subscribe_callback, string_hash,
                     std::equal_to<>>
      _subscribe_map;
};
