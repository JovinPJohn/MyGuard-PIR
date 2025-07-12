# MyGuard-PIR

👁️‍🗨️ A smart inactivity alert system using ESP32 + PIR motion sensor + Web Dashboard

## 📋 Overview

- Detects motion using PIR sensor
- Web interface to set active schedule (Start & End time)
- Resets motion count at start
- If no motion is detected during schedule, triggers buzzer alert in last 2 minutes
- Motion within the alert window cancels buzzer
- Manual buzzer control via web
- Status page to monitor motion and alert status
- Time synced via NTP

## 🛠️ Hardware Used

- ESP32 Dev Board  
- PIR Motion Sensor (HC-SR501)  
- Buzzer  
- LED for alert indication  
- Wi-Fi (for web dashboard)

## 🌐 Features

- ESP32 Web Server (Port 80)
- HTML Form to set schedule
- Manual control for buzzer
- Live status check
- mDNS support (`http://MyGuard.local`)
- Time sync via NTP (`IST`)

## 🔧 Setup

1. Update your Wi-Fi credentials:
   ```cpp
   const char* ssid = "YourWiFi";
   const char* password = "YourPassword";
