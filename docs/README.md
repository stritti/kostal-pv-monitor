---
home: true
heroImage: /img/kostal-pv-monitor.jpg
tagline: ESP32 based Device to monitor Power Consumption of Kostal Plenticore Converters

features:
  - title: ESP32 based E-Ink Device
    details:
      2,13" E-Ink Display with 128x64 pixel resolution showing data with very low energy
      consumption.
  - title: Easy Power Monitoring
    details:
      The device is powered by a LiPo battery and could be placed anywhere.
      Data is transfered using WiFi.
  - title: Modbus
    details:
      Communication is based on official Kostal Plenticore Modbus protocol to load data from Converter.

footer: <div>MIT Licensed | Copyright 2022-present | <a href="https://twitter.com/_stritti_">Stephan Strittmatter</a></div>
footerHtml: true
---

# Power Monitor for Kostal Plenticore Converter

This gadget is designed to show the power usage of a Kostal Plenticore Converter having attached solar cells and buffer battery.

Data is transfered using WiFi and read from Kostal Plenticore Converter using Modbus protocol.

::: warning
This is a private project and not officially supported by [Kostal Solar Electric](https://www.kostal-solar-electric.com/).
:::

![Display of Kostal Plenticore PV Monitor](/img/kostal-pv-monitor-display.jpg)

It displays current power generation and consumption:

* PV production [W]
* Battery charge/discharge [W] and SoC of battery [%]
* House consumption [W]
* Grid consumption/generation [W]
* Time of last query

To get fast insight on power consumtion of the house, the device shows smiley depending on primary energy source:

* battery: 🙂
* PV: 😎
* grid:🙁

## Setup

First of all, please verify if Modbus is activated at your converter:

* Navigate within menu of converter: `Settings` -> `Modbus/Sunspec`
* Pay attention that Modbus is **activated**.

![Modbus settings](/img/modbus-settings.png)

### Assemble Device

1. Upload firmware
2. Connect battery
3. Fit to enclosure

### Initial Start

<img src="/img/kostal-pv-monitor-settings.jpg" alt="Kostal PV Monitor Settings" width="300" align="right">

If you start the device firsst time, it showsinformation to conect to its own WiFi hotspot. The Device runs in AP mode.

Take for example your smartphone and connect to shown WiFi hotspot.

In upcoming screen of the hotspot you are able to select Wifi network and password to attach the device to your home network. You have also to add hostname and port of your converter.

After all values are added, press `Save` button to store the values permanently on the device.

Now you could **restart device** and it will connect to your selected home network and can be used to monitor power consumption.

#### Required Settings

* `WiFi SSID`: SSID/Name of your WiFi network
* `WiFi password`: Password of your WiFi network

* `Kostal Hostname`: Hostname of your Kostal Plenticore Converter
* `Kostal Modbus Port`: Port of your Kostal Plenticore Converter (default: 1502)


## Development

### Parts List (BOM)

* 1 [LILYGO® TTGO T5 V2.0 e-Paper Display Development Board](https://banggood.onelink.me/zMT7/f5eac6bc)
* 1 [TTGO T5 2.13" ePaper Snap Fit Enclosure](https://www.thingiverse.com/thing:4055993): Could be printed at [Craftcloud](https://craftcloud3d.com/) using white ABS material.
* 3.7 V LiPo Battery


### TTGO T5 V2.0 e-Paper Display Development Board

More information about the board could be found at: [GitHub](https://github.com/Xinyuan-LilyGO/T5-Ink-Screen-Series)


### Modbus

Communication is based on [Kostal Plenticore Modbus](https://www.kostal-solar-electric.com/de-de/download/-/media/document-library-folder---kse/2020/12/15/13/38/ba_kostal-interface-description-modbus-tcp_sunspec_hybrid.pdf/)


## Credits

### DFRobot

Project was initiated on [2022 ESP32 Initiation Program: "Micro-Control" Your World](https://community.dfrobot.com/makelog-312165.html). Many thanks to [DFRobot](https://www.dfrobot.com/index.html) for the support.

<img src="/img/kostal-pv-monitor-096-oled.jpg" alt="First version using Monochrome 0.96 128x64 I2C/SPI OLED Display" width="300" align="right">

First version was based on:

* [DFRobot Monochrome 0.96" 128x64 I2C/SPI OLED Display](https://www.dfrobot.com/product-2017.html)
* [DFRobot Gravity: IO Shield for FireBeetle M0 and ESP32-E](https://www.dfrobot.com/product-2395.html)
* [DFRobot FireBeetle ESP32-E IoT Microcontroller with Header](https://www.dfrobot.com/product-2231.html)

The version showed the following data:

* Battery SoC (0-100%)
* Own home consumption from PV (W)
* Own home consumption from grid (W)
* Own home consumption from battery (W)
* Smiley depending on primary energy source


## License

This project is licensed under the [MIT License](https://github.com/stritti/kostal-pv-monitor/blob/main/LICENSE).
