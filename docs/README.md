---
home: true
heroImage: /img/kostal-pv-monitor.jpg
tagline: ESP32 based Device to monitor Power usage of a Kostal Plenticore Converters

features:
  - title: ESP32 based E-Ink Device
    details:
      2,13" E-Ink Display with 128x64 pixel resolution showing data with very low energy
      consumption.
  - title: Power Monitoring everywhere
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

![Display of Kostal Plenticore PV Monitor](/img/kostal-pv-monitor-display.jpg)

It displays current power generation and consumption:

* PV production [W]
* Battery charge/discharge [W] and SoC of battery [%]
* House consumption [W]
* Grid consumption/generation [W]

To get fast insight on power consumtion of the house, the device shows smiley depending on primary energy source:

* battery: 🙂
* PV: 😎
* grid:🙁

## Setup

## Credits

## License
