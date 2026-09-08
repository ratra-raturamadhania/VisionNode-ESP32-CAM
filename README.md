# VisionNode 📷

A lightweight **IoT wireless monitoring system** built using ESP32-CAM with a custom web-based dashboard.

VisionNode was developed as a small IoT project to explore how an ESP32-CAM can operate as a standalone monitoring device through a local Wi-Fi network without requiring an additional application.

## ✨ Features

- Real-time camera streaming
- Image capture with preview
- Flash LED control
- Camera resolution control
- Device uptime monitoring
- Wi-Fi signal strength monitoring
- Local IP information
- Responsive web dashboard

## 🖥️ Dashboard Preview

![VisionNode Dashboard](visionnode-dashboard.png)

## 🏗️ System Architecture

```text
                 Wi-Fi Network
                      │
                      │
                ┌─────▼─────┐
                │ ESP32-CAM │
                └─────┬─────┘
                      │
              Embedded Web Server
                      │
          ┌───────────┼───────────┐
          │           │           │
          ▼           ▼           ▼
     Live Stream   Capture    Device Status
          │           │           │
          └───────────┼───────────┘
                      │
                      ▼
               Web Dashboard
                      │
                      ▼
               User's Browser
```

The ESP32-CAM connects to a Wi-Fi network and hosts its own lightweight HTTP server. The browser communicates directly with the ESP32-CAM to access the camera stream, capture images, control the flash LED, and retrieve device information.

## 🛠️ Hardware

- ESP32-CAM AI Thinker
- USB-to-TTL / FTDI programmer
- Wi-Fi connection

No additional sensors are required.

## 💻 Technologies

- ESP32
- Arduino IDE
- C++
- HTML
- CSS
- JavaScript
- HTTP Web Server

## 🚀 How to Run

1. Open `VisionNode.ino` using Arduino IDE.
2. Select **AI Thinker ESP32-CAM** as the board.
3. Enter your Wi-Fi SSID and password in the following section:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

4. Connect the ESP32-CAM to the computer using a USB-to-TTL/FTDI programmer.
5. Upload the program to the ESP32-CAM.
6. Open the Serial Monitor at **115200 baud**.
7. Wait until the ESP32-CAM connects to Wi-Fi.
8. Copy the displayed local IP address.
9. Open the IP address using a web browser.

Example:

```text
http://192.168.x.x
```

The VisionNode dashboard should now be accessible from devices connected to the same local network.

## 📡 How It Works

VisionNode uses the ESP32-CAM as both the camera device and web server.

Instead of sending camera data to a cloud service, the device provides the monitoring interface directly through the local network.

This allows users to:

- Monitor the camera feed in real time
- Capture an image
- Control the onboard flash LED
- Change camera resolution
- Monitor device uptime
- Check Wi-Fi signal strength

## 🔒 Network Scope

VisionNode currently operates within a **local Wi-Fi network**.

The dashboard is accessible only by devices connected to the same network as the ESP32-CAM.

## 👩‍💻 Author

**Ratu Ramadhania**

Informatics Engineering graduate interested in **IoT, embedded systems, hardware experimentation, and technology**.

---

Built with ESP32-CAM as a personal IoT exploration project.
