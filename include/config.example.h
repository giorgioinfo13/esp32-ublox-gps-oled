#pragma once

// Copy this file to include/config.h and replace the placeholder values
// with your own local Wi-Fi configuration. Never commit include/config.h.

struct WifiNetworkConfig {
  const char *ssid;
  const char *password;
};

constexpr WifiNetworkConfig WIFI_NETWORKS[] = {
    {"YOUR_WIFI_SSID_1", "YOUR_WIFI_PASSWORD_1"},
    {"YOUR_WIFI_SSID_2", "YOUR_WIFI_PASSWORD_2"},
};

constexpr size_t WIFI_NETWORK_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);
