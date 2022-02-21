# Kostal Plenticore Converter Monitor

This is a ESP32 based device that can be used to monitor the power usage of a Kostal Plenticore converter:

* Battery SoC (0-100%)
* Own home consumption from PV (W)
* Own home consumption from grid (W)
* Own home consumption from battery (W)

Data is shown on 0.96" OLED display:

![OLED display](docs/img/kostal-pv-monitor-096-oled.jpg)

## Configuration

Copy `private_config.template.ini` to `private_config.ini` and fill in the values:

* `wifi_ssid`: SSID/Name of your WiFi network

* `wifi_pwd`: Password of your WiFi network

* `kostal_modbus_hostname`: Hostname of your Kostal Plenticore Converter
* `kostal_modbus_port`: Port of your Kostal Plenticore Converter (default: 1502)
* `kostal_modbus_slave_id`: Slave ID of your Kostal Plenticore Converter (default: 71)

Data of the **Kostal Plenticore Modbus** could be found in settings of your Kostal Plenticore Converter:

* Navigate within menu: `Settings` -> `Modbus/Sunspec`
* Pay attention that Modbus is activated.

![Modbus settings](/docs/img/modbus-settings.png)

More Information read [Convention for compile time configuration of PlatformIO projects](https://blog.yavilevich.com/2020/09/convention-for-compile-time-configuration-of-platformio-projects/)

## Development

### PlatformIO

Project is developed with [PlatformIO](https://platformio.org/).

### Modbus

Communication is based on [Kostal Plenticore Modbus](https://www.kostal-solar-electric.com/de-de/download/-/media/document-library-folder---kse/2020/12/15/13/38/ba_kostal-interface-description-modbus-tcp_sunspec_hybrid.pdf/)

## License

[MIT License](LICENSE)
