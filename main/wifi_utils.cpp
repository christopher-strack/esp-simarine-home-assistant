#include "wifi_utils.hpp"

#include "esp_event.h"
#include "esp_netif_types.h"

const char* wifi_disconnect_reason_string(const wifi_err_reason_t reason) {
  switch (reason) {
  case WIFI_REASON_UNSPECIFIED:
    return "unspecified";
  case WIFI_REASON_AUTH_EXPIRE:
    return "auth_expire";
  case WIFI_REASON_AUTH_LEAVE:
    return "auth_leave";
  case WIFI_REASON_ASSOC_EXPIRE:
    return "assoc_expire";
  case WIFI_REASON_ASSOC_TOOMANY:
    return "assoc_toomany";
  case WIFI_REASON_NOT_AUTHED:
    return "not_authed";
  case WIFI_REASON_NOT_ASSOCED:
    return "not_assoced";
  case WIFI_REASON_ASSOC_LEAVE:
    return "assoc_leave";
  case WIFI_REASON_ASSOC_NOT_AUTHED:
    return "assoc_not_authed";
  case WIFI_REASON_DISASSOC_PWRCAP_BAD:
    return "disassoc_pwrcap_bad";
  case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
    return "disassoc_supchan_bad";
  case WIFI_REASON_BSS_TRANSITION_DISASSOC:
    return "bss_transition_disassoc";
  case WIFI_REASON_IE_INVALID:
    return "ie_invalid";
  case WIFI_REASON_MIC_FAILURE:
    return "mic_failure";
  case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    return "4way_handshake_timeout";
  case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
    return "group_key_update_timeout";
  case WIFI_REASON_IE_IN_4WAY_DIFFERS:
    return "ie_in_4way_differs";
  case WIFI_REASON_GROUP_CIPHER_INVALID:
    return "group_cipher_invalid";
  case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
    return "pairwise_cipher_invalid";
  case WIFI_REASON_AKMP_INVALID:
    return "akmp_invalid";
  case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
    return "unsupp_rsn_ie_version";
  case WIFI_REASON_INVALID_RSN_IE_CAP:
    return "invalid_rsn_ie_cap";
  case WIFI_REASON_802_1X_AUTH_FAILED:
    return "802_1x_auth_failed";
  case WIFI_REASON_CIPHER_SUITE_REJECTED:
    return "cipher_suite_rejected";
  case WIFI_REASON_TDLS_PEER_UNREACHABLE:
    return "tdls_peer_unreachable";
  case WIFI_REASON_TDLS_UNSPECIFIED:
    return "tdls_unspecified";
  case WIFI_REASON_SSP_REQUESTED_DISASSOC:
    return "ssp_requested_disassoc";
  case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:
    return "no_ssp_roaming_agreement";
  case WIFI_REASON_BAD_CIPHER_OR_AKM:
    return "bad_cipher_or_akm";
  case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:
    return "not_authorized_this_location";
  case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:
    return "service_change_percludes_ts";
  case WIFI_REASON_UNSPECIFIED_QOS:
    return "unspecified_qos";
  case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:
    return "not_enough_bandwidth";
  case WIFI_REASON_MISSING_ACKS:
    return "missing_acks";
  case WIFI_REASON_EXCEEDED_TXOP:
    return "exceeded_txop";
  case WIFI_REASON_STA_LEAVING:
    return "sta_leaving";
  case WIFI_REASON_END_BA:
    return "end_ba";
  case WIFI_REASON_UNKNOWN_BA:
    return "unknown_ba";
  case WIFI_REASON_TIMEOUT:
    return "timeout";
  case WIFI_REASON_PEER_INITIATED:
    return "peer_initiated";
  case WIFI_REASON_AP_INITIATED:
    return "ap_initiated";
  case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:
    return "invalid_ft_action_frame_count";
  case WIFI_REASON_INVALID_PMKID:
    return "invalid_pmkid";
  case WIFI_REASON_INVALID_MDE:
    return "invalid_mde";
  case WIFI_REASON_INVALID_FTE:
    return "invalid_fte";
  case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED:
    return "transmission_link_establish_failed";
  case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:
    return "alterative_channel_occupied";
  case WIFI_REASON_BEACON_TIMEOUT:
    return "beacon_timeout";
  case WIFI_REASON_NO_AP_FOUND:
    return "no_ap_found";
  case WIFI_REASON_AUTH_FAIL:
    return "auth_fail";
  case WIFI_REASON_ASSOC_FAIL:
    return "assoc_fail";
  case WIFI_REASON_HANDSHAKE_TIMEOUT:
    return "handshake_timeout";
  case WIFI_REASON_CONNECTION_FAIL:
    return "connection_fail";
  case WIFI_REASON_AP_TSF_RESET:
    return "ap_tsf_reset";
  case WIFI_REASON_ROAMING:
    return "roaming";
  case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:
    return "assoc_comeback_time_too_long";
  case WIFI_REASON_SA_QUERY_TIMEOUT:
    return "sa_query_timeout";
  case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
    return "no_ap_found_w_compatible_security";
  case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
    return "no_ap_found_in_authmode_threshold";
  case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
    return "no_ap_found_in_rssi_threshold";
  }

  return "unknown";
}
