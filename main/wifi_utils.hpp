#pragma once

#include <future>

#include "esp_event_base.h"
#include "esp_wifi_types_generic.h"

const char* wifi_disconnect_reason_string(const wifi_err_reason_t reason);
