<!-- Generate Readme file for this project -->

# Smart Garden watering system - ESP8266 + MQTT

<!-- Badges - Arduino, C++, PlatformIO, ESP8266 -->
![Arduino CI](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)
![C++ CI](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![PlatformIO CI](https://img.shields.io/static/v1?label=Build&message=PlatformIO&color=orange&style=for-the-badge)
![ESP8266 CI](https://img.shields.io/static/v1?label=ESP8266&message=NodeMCU&logo=espressif&color=red&style=for-the-badge)
![License](https://img.shields.io/github/license/amjed-ali-k/WaterPumpAutomation?style=for-the-badge)
[![Bitcoin](https://img.shields.io/badge/Bitcoin-F7931A?logo=bitcoin&logoColor=fff&style=for-the-badge
)](bitcoin://bc1qum3cr9cj7lxkyqgsljzphcugaprr3thhk5yl8r)

## Description

The Smart Garden watering system is a comprehensive solution designed to automate and optimize garden irrigation. Utilizing solenoid valves, it efficiently controls water flow. The valves open anytime daily, (Eg. 7:00 AM and 5:00 PM) allowing water to flow for 5 minutes or until the moisture sensor detects adequate water saturation. If moisture is detected, the valve closes and the next valve in line opens. Moisture sensor readings occur every 5 minutes and are transmitted to the cloud for analysis.

The system intelligently skips watering if the soil moisture levels are already sufficient. The configuration file, including watering times, is fetched from the cloud upon device restart. Conveniently, the valves can be manually controlled, either opened or closed, via the cloud interface. Furthermore, the entire system can be remotely turned off through cloud connectivity. MQTT is employed as the communication protocol, facilitating efficient and reliable data exchange between the garden watering system and the cloud. With its automated functionality and cloud integration, the Smart Garden watering system ensures optimal water management for healthy and flourishing gardens.

## Hardware

- ESP8266
- PCF8575 GPIO Expander
- Relay module
- 12V power supply
- Solenoid Valve
- Moisture sensor

## Software

- PlatformIO
- Arduino framework
- VSCode
