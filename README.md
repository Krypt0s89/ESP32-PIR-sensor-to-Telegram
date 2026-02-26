<h1 align="center">🚨 ESP32 PIR Motion Telegram Notifier</h1>

<p align="center">
  <b>Motion detection with ESP32 + Telegram alerts + LED indication</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/ESP--IDF-v5.x-blue" />
  <img src="https://img.shields.io/badge/Platform-ESP32-red" />
  <img src="https://img.shields.io/badge/FreeRTOS-Enabled-green" />
  <img src="https://img.shields.io/badge/License-MIT-yellow" />
</p>

<hr>

<h2>📌 Overview</h2>

<p>
This project uses an <b>ESP32</b> to detect motion via a <b>PIR sensor</b>.
When motion is detected:
</p>

<ul>
  <li>💡 LED blinks for 5 seconds</li>
  <li>📩 A Telegram notification is sent</li>
  <li>🕒 Message includes timestamp (GMT+2 with DST)</li>
</ul>

<p>
Built with <b>ESP-IDF</b>, using FreeRTOS tasks, GPIO interrupts (ISR),
WiFi connectivity, and SNTP time synchronization.
</p>

<hr>

<h2>✨ Features</h2>

<ul>
  <li>🔔 GPIO interrupt-based PIR detection</li>
  <li>💡 Non-blocking LED blinking task</li>
  <li>📡 Telegram Bot API integration</li>
  <li>🧵 FreeRTOS multitasking architecture</li>
  <li>🔒 Thread-safe HTTP via mutex</li>
  <li>⏱ Anti-spam protection (10s delay)</li>
  <li>⚙️ Credentials configurable via <code>menuconfig</code></li>
</ul>

<hr>

<h2>🏗 Architecture</h2>

<pre>
        PIR Sensor (GPIO4)
                │
                ▼
              ISR
                │
        ┌───────┴────────┐
        ▼                ▼
   LED Task        Telegram Task
 (5s blinking)    (HTTP request)
</pre>

<hr>

<h2>🧰 Hardware</h2>

<ul>
  <li>ESP32 Dev Kit</li>
  <li>PIR Motion Sensor (GPIO4)</li>
  <li>LED (GPIO2)</li>
</ul>

<hr>

<h2>🔌 Wiring</h2>

<table>
  <tr>
    <th>Component</th>
    <th>GPIO</th>
  </tr>
  <tr>
    <td>PIR Sensor OUT</td>
    <td>GPIO4</td>
  </tr>
  <tr>
    <td>LED (+)</td>
    <td>GPIO2</td>
  </tr>
  <tr>
    <td>GND</td>
    <td>GND</td>
  </tr>
  <tr>
    <td>3.3V</td>
    <td>3.3V</td>
  </tr>
</table>

<hr>

<h2>🚀 Quick Setup</h2>

<h3>1️⃣ Clone project</h3>

<pre>
git clone &lt;your-repository-url&gt;
cd &lt;project-folder&gt;
idf.py menuconfig
</pre>

<h3>2️⃣ Configure credentials</h3>

<p>Open <b>User Settings</b> and set:</p>

<ul>
  <li><code>WIFI_SSID</code></li>
  <li><code>WIFI_PASSWORD</code></li>
  <li><code>BOT_TOKEN</code></li>
  <li><code>CHAT_ID</code></li>
</ul>

<h3>3️⃣ Build and flash</h3>

<pre>
idf.py build
idf.py flash
idf.py monitor
</pre>

<hr>

<h2>⚙️ Configuration (menuconfig)</h2>

<pre>
User Settings  --->
    WiFi SSID
    WiFi Password
    Telegram Bot Token
    Telegram Chat ID
</pre>

<hr>

<h2>📝 Implementation Notes</h2>

<ul>
  <li>ISR is registered immediately after GPIO configuration.</li>
  <li>Telegram URLs are URL-encoded (spaces → %20, ":" → %3A).</li>
  <li>LED timer resets if motion occurs again within 5 seconds.</li>
  <li>Telegram notifications are limited to 1 message per 10 seconds.</li>
  <li>SNTP used for automatic time synchronization.</li>
</ul>

<hr>

<h2>📜 License</h2>

<p>
This project is licensed under the <b>MIT License</b>.
</p>
