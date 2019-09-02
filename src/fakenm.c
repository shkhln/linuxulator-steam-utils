#include <assert.h>
#include <stdio.h>

#define FAKE(name) void name() { fprintf(stderr, "fakenm: %s\n", #name); assert(0); }

FAKE(nm_access_point_get_flags);
FAKE(nm_access_point_get_rsn_flags);
FAKE(nm_access_point_get_ssid);
FAKE(nm_access_point_get_strength);
FAKE(nm_access_point_get_wpa_flags);
FAKE(nm_active_connection_get_default);
FAKE(nm_active_connection_get_specific_object);
FAKE(nm_client_activate_connection);
FAKE(nm_client_add_and_activate_connection);
FAKE(nm_client_get_devices);
FAKE(nm_client_networking_get_enabled);
FAKE(nm_client_networking_set_enabled);
FAKE(nm_client_wireless_get_enabled);
FAKE(nm_client_wireless_set_enabled);
FAKE(nm_connection_add_setting);
FAKE(nm_connection_compare);
FAKE(nm_connection_dump);
FAKE(nm_connection_get_setting_connection);
FAKE(nm_connection_get_setting_wireless);
FAKE(nm_connection_get_type);
FAKE(nm_connection_is_type);
FAKE(nm_connection_need_secrets);
FAKE(nm_connection_new);
FAKE(nm_connection_replace_settings);
FAKE(nm_connection_to_hash);
FAKE(nm_connection_verify);
FAKE(nm_device_disconnect);
FAKE(nm_device_ethernet_get_carrier);
FAKE(nm_device_ethernet_get_hw_address);
FAKE(nm_device_ethernet_get_permanent_hw_address);
FAKE(nm_device_ethernet_get_type);
FAKE(nm_device_filter_connections);
FAKE(nm_device_get_active_connection);
FAKE(nm_device_get_dhcp4_config);
FAKE(nm_device_get_ip4_config);
FAKE(nm_device_get_product);
FAKE(nm_device_get_state);
FAKE(nm_device_get_udi);
FAKE(nm_device_get_vendor);
FAKE(nm_device_wifi_get_access_points);
FAKE(nm_device_wifi_get_active_access_point);
FAKE(nm_device_wifi_get_capabilities);
FAKE(nm_device_wifi_get_hw_address);
FAKE(nm_device_wifi_get_permanent_hw_address);
FAKE(nm_device_wifi_get_type);
FAKE(nm_ip4_address_get_address);
FAKE(nm_ip4_address_get_gateway);
FAKE(nm_ip4_address_get_prefix);
FAKE(nm_ip4_address_new);
FAKE(nm_ip4_address_set_address);
FAKE(nm_ip4_address_set_gateway);
FAKE(nm_ip4_address_set_prefix);
FAKE(nm_ip4_address_unref);
FAKE(nm_ip4_config_get_addresses);
FAKE(nm_ip4_config_get_nameservers);
FAKE(nm_remote_connection_commit_changes);
FAKE(nm_remote_connection_delete);
FAKE(nm_remote_connection_get_secrets);
FAKE(nm_remote_connection_get_type);
FAKE(nm_remote_settings_list_connections);
FAKE(nm_remote_settings_new);
FAKE(nm_setting_connection_get_autoconnect);
FAKE(nm_setting_connection_get_id);
FAKE(nm_setting_connection_new);
FAKE(nm_setting_ip4_config_add_address);
FAKE(nm_setting_ip4_config_add_dns);
FAKE(nm_setting_ip4_config_new);
FAKE(nm_setting_ip6_config_new);
FAKE(nm_setting_wired_new);
FAKE(nm_setting_wireless_get_ssid);
FAKE(nm_setting_wireless_new);
FAKE(nm_setting_wireless_security_new);
FAKE(nm_utils_ip4_netmask_to_prefix);
FAKE(nm_utils_ip4_prefix_to_netmask);
FAKE(nm_utils_security_valid);
FAKE(nm_utils_ssid_to_utf8);
FAKE(nm_utils_uuid_generate);

int nm_client_get_type() {
  return 0;
}
