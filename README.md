===== ESP32 PIR Motion Telegram Notifier =====

This ESP32 project detects motion using a PIR sensor, blinks an LED, and sends a Telegram notification with the timestamp of the event. It uses FreeRTOS tasks, ISR handling, WiFi, and SNTP time synchronization.

==============================================
Features:

PIR motion detection via GPIO interrupt
LED blinks for 5 seconds on motion
Sends Telegram messages with GMT+2 time
Configurable WiFi and Telegram credentials via menuconfig
Thread-safe HTTP requests using mutexes

==============================================
Hardware:

ESP32 Dev Kit
PIR sensor (GPIO4)
LED (GPIO2)

==============================================
Quick Setup

Clone and open project:
git clone <repo-url>
cd <repo-folder>
idf.py menuconfig

Set WiFi credentials and Telegram bot info in User Settings:
WIFI_SSID
WIFI_PASSWORD
BOT_TOKEN
CHAT_ID

Build and flash:
idf.py build
idf.py flash
idf.py monitor

Wiring:
Component	GPIO
PIR Sensor	4
LED	2

==============================================
Notes

ISR must be added immediately after GPIO configuration.
Telegram URLs are URL-encoded to handle spaces and colons.
LED task resets timer if multiple triggers occur within 5 seconds.
Telegram messages are rate-limited (10s anti-spam).

==============================================
License
MIT License