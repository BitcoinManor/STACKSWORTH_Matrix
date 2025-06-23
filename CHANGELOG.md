# CHANGELOG

All notable changes to this project will be documented in this file.

## [v1.0.2] - 2025-06-23
### Added
- Device MAC address now embedded in the Access Point name (SSID)
- MAC fragment shown under the logo inside the captive portal
- Unified MAC format across SSID, HTML portal, and router visibility

### Improved
- Ensured matching MAC output using `esp_wifi_get_mac(WIFI_IF_STA, ...)`
- Clear and consistent device identification for all STACKSWORTH units

### Notes
- This version enables MAC-based device tracking for future manifest support
- Helps distinguish multiple devices in the same environment
