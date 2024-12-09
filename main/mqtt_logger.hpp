#pragma once

#include "mqtt_client.hpp"

#include <array>

void setup_mqtt_logger(mqtt_client& client);

void send_mqtt_logger_device_discovery();
