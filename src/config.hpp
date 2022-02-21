#pragma once

// check (required) parameters passed from the ini
// create your own private_config.ini with the data. See private_config.template.ini

#ifndef WIFI_SSID
#error Need to define WIFI_SSID
#endif

#ifndef WIFI_PWD
#error Need to define WIFI_PWD
#endif

#ifndef KOSTAL_MODBUS_HOSTNAME
#error Need to define KOSTAL_MODBUS_HOSTNAME
#endif

#ifndef KOSTAL_MODBUS_PORT
#error Need to define KOSTAL_MODBUS_PORT
#endif

#ifndef KOSTAL_MODBUS_SLAVE_ID
#error Need to define KOSTAL_MODBUS_SLAVE_ID
#endif
