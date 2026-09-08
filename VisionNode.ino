#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <time.h>

// =====================================================
// WIFI
// =====================================================
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// =====================================================
// ESP32-CAM AI THINKER PINS
// =====================================================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN      4

// =====================================================
// SERVER
// =====================================================
httpd_handle_t web_server = NULL;
httpd_handle_t stream_server = NULL;

unsigned long bootTime = 0;

// =====================================================
// LAST CAPTURE
// =====================================================
uint8_t* lastCaptureBuffer = NULL;
size_t lastCaptureLength = 0;

String lastCaptureTime = "No capture yet";

// =====================================================
// STREAM
// =====================================================
#define PART_BOUNDARY "123456789000000000000987654321"

static const char* STREAM_CONTENT_TYPE =
  "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static const char* STREAM_BOUNDARY =
  "\r\n--" PART_BOUNDARY "\r\n";

static const char* STREAM_PART =
  "Content-Type: image/jpeg\r\n"
  "Content-Length: %u\r\n\r\n";

// =====================================================
// WIFI QUALITY
// =====================================================
String getWifiQuality(int rssi) {

  if (rssi >= -50) {
    return "Excellent";
  }

  if (rssi >= -60) {
    return "Good";
  }

  if (rssi >= -70) {
    return "Fair";
  }

  return "Weak";
}

// =====================================================
// CURRENT TIME
// =====================================================
String getCurrentTimeString() {

  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 300)) {

    char buffer[40];

    strftime(
      buffer,
      sizeof(buffer),
      "%d %b %Y - %H:%M:%S",
      &timeinfo
    );

    return String(buffer);
  }

  // fallback kalau internet time belum berhasil
  unsigned long totalSeconds =
    (millis() - bootTime) / 1000;

  unsigned long hours =
    totalSeconds / 3600;

  unsigned long minutes =
    (totalSeconds % 3600) / 60;

  unsigned long seconds =
    totalSeconds % 60;

  char buffer[30];

  snprintf(
    buffer,
    sizeof(buffer),
    "Uptime %02lu:%02lu:%02lu",
    hours,
    minutes,
    seconds
  );

  return String(buffer);
}

// =====================================================
// DASHBOARD HTML
// =====================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>

<meta charset="UTF-8">

<meta
name="viewport"
content="width=device-width, initial-scale=1.0">

<title>VisionNode | Ratu Ramadhania</title>

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, Helvetica, sans-serif;
  background: #f3f4f6;
  color: #111827;
}

.container {
  width: 92%;
  max-width: 900px;
  margin: 35px auto;
}

.card {
  background: #ffffff;
  padding: 24px;
  border-radius: 18px;
  margin-bottom: 20px;
  box-shadow: 0 8px 25px rgba(0,0,0,.07);
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 20px;
}

h1 {
  margin: 0;
  font-size: 30px;
}

.subtitle {
  color: #6b7280;
  margin-top: 7px;
  font-size: 14px;
}

.author {
  color: #9ca3af;
  margin-top: 6px;
  font-size: 12px;
}

.author b {
  color: #4b5563;
}

.status {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: bold;
  font-size: 13px;
}

.dot {
  width: 10px;
  height: 10px;
  background: #22c55e;
  border-radius: 50%;
  box-shadow: 0 0 8px rgba(34,197,94,.7);
}

.camera-box {
  margin-top: 22px;
  background: #111;
  border-radius: 14px;
  overflow: hidden;
  min-height: 250px;
  display: flex;
  justify-content: center;
  align-items: center;
}

.camera-box img {
  display: block;
  width: 100%;
  height: auto;
}

.section-title {
  margin-top: 0;
  margin-bottom: 18px;
  font-size: 18px;
}

.info-grid {
  display: grid;
  grid-template-columns: repeat(3,1fr);
  gap: 12px;
}

.info {
  background: #f9fafb;
  border: 1px solid #eeeeee;
  padding: 17px;
  border-radius: 12px;
}

.label {
  font-size: 12px;
  color: #6b7280;
}

.value {
  font-size: 18px;
  font-weight: bold;
  margin-top: 7px;
}

.small-value {
  font-size: 13px;
  color: #6b7280;
  margin-top: 4px;
}

.controls {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

button,
select {
  border: none;
  padding: 12px 17px;
  border-radius: 10px;
  font-size: 14px;
}

button {
  background: #111827;
  color: white;
  font-weight: bold;
  cursor: pointer;
  transition: .2s;
}

button:hover {
  opacity: .85;
  transform: translateY(-1px);
}

.secondary {
  background: #e5e7eb;
  color: #111827;
}

select {
  background: #e5e7eb;
  cursor: pointer;
}

.message {
  margin-top: 14px;
  min-height: 18px;
  color: #6b7280;
  font-size: 13px;
}

.capture-preview {
  width: 100%;
  max-width: 550px;
  border-radius: 14px;
  display: none;
  margin-top: 15px;
  background: #111;
}

.capture-time {
  color: #6b7280;
  font-size: 13px;
}

.footer {
  text-align: center;
  color: #9ca3af;
  font-size: 12px;
  padding: 10px;
}

@media(max-width: 650px) {

  .header {
    flex-direction: column;
    align-items: flex-start;
  }

  .info-grid {
    grid-template-columns: 1fr;
  }

}

</style>

</head>

<body>

<div class="container">

<!-- =================================================
     HEADER + LIVE STREAM
================================================== -->

<div class="card">

<div class="header">

<div>

<h1>VisionNode</h1>

<div class="subtitle">
ESP32-CAM Wireless Monitoring System
</div>

<div class="author">
Developed by <b>Ratu Ramadhania</b>
</div>

</div>

<div class="status">

<div class="dot"></div>

DEVICE ONLINE

</div>

</div>

<div class="camera-box">

<img
id="stream"
alt="ESP32-CAM Live Stream">

</div>

</div>

<!-- =================================================
     DEVICE INFORMATION
================================================== -->

<div class="card">

<h2 class="section-title">
Device Information
</h2>

<div class="info-grid">

<div class="info">

<div class="label">
IP ADDRESS
</div>

<div
class="value"
id="ip">

-

</div>

</div>

<div class="info">

<div class="label">
UPTIME
</div>

<div
class="value"
id="uptime">

-

</div>

</div>

<div class="info">

<div class="label">
WI-FI SIGNAL
</div>

<div
class="value"
id="rssi">

-

</div>

<div
class="small-value"
id="wifiQuality">

-

</div>

</div>

</div>

</div>

<!-- =================================================
     CAMERA CONTROL
================================================== -->

<div class="card">

<h2 class="section-title">
Camera Control
</h2>

<div class="controls">

<button onclick="captureImage()">
Capture Image
</button>

<button onclick="flashOn()">
Flash ON
</button>

<button
class="secondary"
onclick="flashOff()">

Flash OFF

</button>

<select
id="resolution"
onchange="setResolution(this.value)">

<option value="5">
320 × 240
</option>

<option
value="6"
selected>

640 × 480

</option>

<option value="8">
800 × 600
</option>

<option value="10">
1600 × 1200
</option>

</select>

</div>

<div
class="message"
id="message">
</div>

</div>

<!-- =================================================
     LAST CAPTURE
================================================== -->

<div class="card">

<h2 class="section-title">
Last Capture
</h2>

<div
class="capture-time"
id="captureTime">

No capture yet

</div>

<img
id="capturePreview"
class="capture-preview"
alt="Last Captured Image">

</div>

<div class="footer">

VisionNode • ESP32-CAM IoT Project • Ratu Ramadhania

</div>

</div>

<!-- =================================================
     JAVASCRIPT
================================================== -->

<script>

const host = window.location.hostname;

// =====================================================
// LIVE STREAM
// =====================================================

document.getElementById("stream").src =
  "http://" + host + ":81/stream";

// =====================================================
// SEND COMMAND
// =====================================================

function sendCommand(variable, value) {

  return fetch(
    "/control?var=" +
    variable +
    "&val=" +
    value
  );

}

// =====================================================
// FLASH
// =====================================================

function flashOn() {

  sendCommand(
    "flash",
    "1"
  );

  document.getElementById(
    "message"
  ).innerText =
    "Flash turned ON.";

}

function flashOff() {

  sendCommand(
    "flash",
    "0"
  );

  document.getElementById(
    "message"
  ).innerText =
    "Flash turned OFF.";

}

// =====================================================
// RESOLUTION
// =====================================================

function setResolution(value) {

  sendCommand(
    "framesize",
    value
  );

  document.getElementById(
    "message"
  ).innerText =
    "Camera resolution updated.";

}

// =====================================================
// CAPTURE
// =====================================================

function captureImage() {

  const preview =
    document.getElementById(
      "capturePreview"
    );

  document.getElementById(
    "message"
  ).innerText =
    "Capturing image...";

  preview.onload = function() {

    preview.style.display =
      "block";

    document.getElementById(
      "message"
    ).innerText =
      "Image captured successfully.";

    setTimeout(
      updateStatus,
      300
    );

  };

  preview.onerror = function() {

    document.getElementById(
      "message"
    ).innerText =
      "Capture failed.";

  };

  preview.src =
    "/capture?ts=" +
    Date.now();

}

// =====================================================
// STATUS
// =====================================================

function updateStatus() {

  fetch(
    "/status?ts=" +
    Date.now()
  )

  .then(
    response =>
      response.json()
  )

  .then(
    data => {

      document.getElementById(
        "ip"
      ).innerText =
        data.ip;

      document.getElementById(
        "uptime"
      ).innerText =
        data.uptime;

      document.getElementById(
        "rssi"
      ).innerText =
        data.rssi +
        " dBm";

      document.getElementById(
        "wifiQuality"
      ).innerText =
        data.wifi_quality;

      document.getElementById(
        "captureTime"
      ).innerText =
        data.last_capture;

      if (
        data.has_capture
      ) {

        const preview =
          document.getElementById(
            "capturePreview"
          );

        if (
          !preview.src ||
          preview.src.indexOf(
            "/capture?"
          ) === -1
        ) {

          preview.src =
            "/last-capture?ts=" +
            Date.now();

        }

        preview.style.display =
          "block";
      }

    }
  )

  .catch(
    error => {

      console.log(
        "Status update failed"
      );

    }
  );

}

updateStatus();

setInterval(
  updateStatus,
  1000
);

</script>

</body>
</html>
)rawliteral";

// =====================================================
// ROOT HANDLER
// =====================================================
static esp_err_t index_handler(
  httpd_req_t *req
) {

  httpd_resp_set_type(
    req,
    "text/html"
  );

  httpd_resp_set_hdr(
    req,
    "Cache-Control",
    "no-store"
  );

  return httpd_resp_send(
    req,
    INDEX_HTML,
    HTTPD_RESP_USE_STRLEN
  );
}

// =====================================================
// STREAM HANDLER
// =====================================================
static esp_err_t stream_handler(
  httpd_req_t *req
) {

  camera_fb_t* fb = NULL;

  esp_err_t res =
    httpd_resp_set_type(
      req,
      STREAM_CONTENT_TYPE
    );

  if (
    res != ESP_OK
  ) {

    return res;
  }

  httpd_resp_set_hdr(
    req,
    "Access-Control-Allow-Origin",
    "*"
  );

  while(true) {

    fb =
      esp_camera_fb_get();

    if (
      !fb
    ) {

      Serial.println(
        "Camera capture failed"
      );

      break;
    }

    char partBuffer[64];

    size_t headerLength =
      snprintf(
        partBuffer,
        sizeof(partBuffer),
        STREAM_PART,
        fb->len
      );

    res =
      httpd_resp_send_chunk(
        req,
        partBuffer,
        headerLength
      );

    if (
      res == ESP_OK
    ) {

      res =
        httpd_resp_send_chunk(
          req,
          (const char*)fb->buf,
          fb->len
        );
    }

    if (
      res == ESP_OK
    ) {

      res =
        httpd_resp_send_chunk(
          req,
          STREAM_BOUNDARY,
          strlen(
            STREAM_BOUNDARY
          )
        );
    }

    esp_camera_fb_return(
      fb
    );

    fb = NULL;

    if (
      res != ESP_OK
    ) {

      break;
    }

    delay(20);
  }

  return res;
}

// =====================================================
// STATUS HANDLER
// =====================================================
static esp_err_t status_handler(
  httpd_req_t *req
) {

  unsigned long totalSeconds =
    (millis() - bootTime)
    / 1000;

  unsigned long hours =
    totalSeconds
    / 3600;

  unsigned long minutes =
    (totalSeconds % 3600)
    / 60;

  unsigned long seconds =
    totalSeconds
    % 60;

  char uptime[24];

  snprintf(
    uptime,
    sizeof(uptime),
    "%02lu:%02lu:%02lu",
    hours,
    minutes,
    seconds
  );

  int currentRSSI =
    WiFi.RSSI();

  String json =
    "{";

  json +=
    "\"ip\":\"";

  json +=
    WiFi.localIP()
    .toString();

  json +=
    "\",";

  json +=
    "\"uptime\":\"";

  json +=
    uptime;

  json +=
    "\",";

  json +=
    "\"rssi\":";

  json +=
    String(
      currentRSSI
    );

  json +=
    ",";

  json +=
    "\"wifi_quality\":\"";

  json +=
    getWifiQuality(
      currentRSSI
    );

  json +=
    "\",";

  json +=
    "\"last_capture\":\"";

  json +=
    lastCaptureTime;

  json +=
    "\",";

  json +=
    "\"has_capture\":";

  if (
    lastCaptureLength > 0
  ) {

    json += "true";

  } else {

    json += "false";
  }

  json +=
    "}";

  httpd_resp_set_type(
    req,
    "application/json"
  );

  httpd_resp_set_hdr(
    req,
    "Cache-Control",
    "no-store"
  );

  return httpd_resp_send(
    req,
    json.c_str(),
    HTTPD_RESP_USE_STRLEN
  );
}

// =====================================================
// CONTROL HANDLER
// =====================================================
static esp_err_t control_handler(
  httpd_req_t *req
) {

  char query[100];

  char variable[32];

  char value[32];

  if (
    httpd_req_get_url_query_str(
      req,
      query,
      sizeof(query)
    )
    != ESP_OK
  ) {

    return
      httpd_resp_send_404(
        req
      );
  }

  if (
    httpd_query_key_value(
      query,
      "var",
      variable,
      sizeof(variable)
    )
    != ESP_OK
  ) {

    return
      httpd_resp_send_404(
        req
      );
  }

  if (
    httpd_query_key_value(
      query,
      "val",
      value,
      sizeof(value)
    )
    != ESP_OK
  ) {

    return
      httpd_resp_send_404(
        req
      );
  }

  // RESOLUTION
  if (
    strcmp(
      variable,
      "framesize"
    )
    == 0
  ) {

    sensor_t* sensor =
      esp_camera_sensor_get();

    if (
      sensor
    ) {

      int frameSize =
        atoi(
          value
        );

      sensor->set_framesize(
        sensor,
        (framesize_t)
        frameSize
      );

      Serial.printf(
        "Resolution changed to: %d\n",
        frameSize
      );
    }
  }

  // FLASH
  else if (
    strcmp(
      variable,
      "flash"
    )
    == 0
  ) {

    int state =
      atoi(
        value
      );

    digitalWrite(
      FLASH_LED_PIN,
      state
        ? HIGH
        : LOW
    );

    Serial.println(
      state
        ? "Flash ON"
        : "Flash OFF"
    );
  }

  else {

    return
      httpd_resp_send_404(
        req
      );
  }

  httpd_resp_set_hdr(
    req,
    "Access-Control-Allow-Origin",
    "*"
  );

  return httpd_resp_send(
    req,
    "OK",
    HTTPD_RESP_USE_STRLEN
  );
}

// =====================================================
// CAPTURE HANDLER
// =====================================================
static esp_err_t capture_handler(
  httpd_req_t *req
) {

  Serial.println(
    "Capture requested"
  );

  camera_fb_t* fb =
    esp_camera_fb_get();

  if (
    !fb
  ) {

    Serial.println(
      "Capture failed"
    );

    httpd_resp_send_500(
      req
    );

    return ESP_FAIL;
  }

  // ==================================================
  // DELETE OLD CAPTURE
  // ==================================================
  if (
    lastCaptureBuffer
    != NULL
  ) {

    free(
      lastCaptureBuffer
    );

    lastCaptureBuffer =
      NULL;

    lastCaptureLength =
      0;
  }

  // ==================================================
  // SAVE NEW CAPTURE TO PSRAM
  // ==================================================
  lastCaptureBuffer =
    (uint8_t*)
    ps_malloc(
      fb->len
    );

  if (
    lastCaptureBuffer
    != NULL
  ) {

    memcpy(
      lastCaptureBuffer,
      fb->buf,
      fb->len
    );

    lastCaptureLength =
      fb->len;

    lastCaptureTime =
      getCurrentTimeString();

    Serial.println(
      "Capture saved to memory"
    );
  }

  else {

    Serial.println(
      "Failed to allocate capture memory"
    );
  }

  Serial.printf(
    "Captured %u bytes\n",
    fb->len
  );

  httpd_resp_set_type(
    req,
    "image/jpeg"
  );

  httpd_resp_set_hdr(
    req,
    "Cache-Control",
    "no-store"
  );

  httpd_resp_set_hdr(
    req,
    "Content-Disposition",
    "inline; filename=VisionNode_RatuRamadhania.jpg"
  );

  esp_err_t result =
    httpd_resp_send(
      req,
      (const char*)fb->buf,
      fb->len
    );

  esp_camera_fb_return(
    fb
  );

  return result;
}

// =====================================================
// LAST CAPTURE HANDLER
// =====================================================
static esp_err_t last_capture_handler(
  httpd_req_t *req
) {

  if (
    lastCaptureBuffer
    == NULL
    ||
    lastCaptureLength
    == 0
  ) {

    return
      httpd_resp_send_404(
        req
      );
  }

  httpd_resp_set_type(
    req,
    "image/jpeg"
  );

  httpd_resp_set_hdr(
    req,
    "Cache-Control",
    "no-store"
  );

  return httpd_resp_send(
    req,
    (const char*)lastCaptureBuffer,
    lastCaptureLength
  );
}

// =====================================================
// START SERVERS
// =====================================================
void startCameraServer() {

  // ==================================================
  // WEB SERVER PORT 80
  // ==================================================
  httpd_config_t webConfig =
    HTTPD_DEFAULT_CONFIG();

  webConfig.server_port =
    80;

  httpd_uri_t index_uri = {

    .uri = "/",

    .method =
      HTTP_GET,

    .handler =
      index_handler,

    .user_ctx =
      NULL
  };

  httpd_uri_t status_uri = {

    .uri =
      "/status",

    .method =
      HTTP_GET,

    .handler =
      status_handler,

    .user_ctx =
      NULL
  };

  httpd_uri_t control_uri = {

    .uri =
      "/control",

    .method =
      HTTP_GET,

    .handler =
      control_handler,

    .user_ctx =
      NULL
  };

  httpd_uri_t capture_uri = {

    .uri =
      "/capture",

    .method =
      HTTP_GET,

    .handler =
      capture_handler,

    .user_ctx =
      NULL
  };

  httpd_uri_t last_capture_uri = {

    .uri =
      "/last-capture",

    .method =
      HTTP_GET,

    .handler =
      last_capture_handler,

    .user_ctx =
      NULL
  };

  Serial.println(
    "Starting dashboard server on port 80..."
  );

  if (
    httpd_start(
      &web_server,
      &webConfig
    )
    == ESP_OK
  ) {

    httpd_register_uri_handler(
      web_server,
      &index_uri
    );

    httpd_register_uri_handler(
      web_server,
      &status_uri
    );

    httpd_register_uri_handler(
      web_server,
      &control_uri
    );

    httpd_register_uri_handler(
      web_server,
      &capture_uri
    );

    httpd_register_uri_handler(
      web_server,
      &last_capture_uri
    );

    Serial.println(
      "Dashboard server started!"
    );
  }

  // ==================================================
  // STREAM SERVER PORT 81
  // ==================================================
  httpd_config_t streamConfig =
    HTTPD_DEFAULT_CONFIG();

  streamConfig.server_port =
    81;

  streamConfig.ctrl_port =
    webConfig.ctrl_port
    + 1;

  httpd_uri_t stream_uri = {

    .uri =
      "/stream",

    .method =
      HTTP_GET,

    .handler =
      stream_handler,

    .user_ctx =
      NULL
  };

  Serial.println(
    "Starting stream server on port 81..."
  );

  if (
    httpd_start(
      &stream_server,
      &streamConfig
    )
    == ESP_OK
  ) {

    httpd_register_uri_handler(
      stream_server,
      &stream_uri
    );

    Serial.println(
      "Stream server started!"
    );
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  WRITE_PERI_REG(
    RTC_CNTL_BROWN_OUT_REG,
    0
  );

  Serial.begin(
    115200
  );

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "VisionNode V4"
  );

  Serial.println(
    "Developed by Ratu Ramadhania"
  );

  Serial.println(
    "================================"
  );

  bootTime =
    millis();

  // ==================================================
  // FLASH LED
  // ==================================================
  pinMode(
    FLASH_LED_PIN,
    OUTPUT
  );

  digitalWrite(
    FLASH_LED_PIN,
    LOW
  );

  // ==================================================
  // CAMERA CONFIG
  // ==================================================
  camera_config_t config;

  config.ledc_channel =
    LEDC_CHANNEL_0;

  config.ledc_timer =
    LEDC_TIMER_0;

  config.pin_d0 =
    Y2_GPIO_NUM;

  config.pin_d1 =
    Y3_GPIO_NUM;

  config.pin_d2 =
    Y4_GPIO_NUM;

  config.pin_d3 =
    Y5_GPIO_NUM;

  config.pin_d4 =
    Y6_GPIO_NUM;

  config.pin_d5 =
    Y7_GPIO_NUM;

  config.pin_d6 =
    Y8_GPIO_NUM;

  config.pin_d7 =
    Y9_GPIO_NUM;

  config.pin_xclk =
    XCLK_GPIO_NUM;

  config.pin_pclk =
    PCLK_GPIO_NUM;

  config.pin_vsync =
    VSYNC_GPIO_NUM;

  config.pin_href =
    HREF_GPIO_NUM;

  config.pin_sscb_sda =
    SIOD_GPIO_NUM;

  config.pin_sscb_scl =
    SIOC_GPIO_NUM;

  config.pin_pwdn =
    PWDN_GPIO_NUM;

  config.pin_reset =
    RESET_GPIO_NUM;

  config.xclk_freq_hz =
    20000000;

  config.pixel_format =
    PIXFORMAT_JPEG;

  // ==================================================
  // PSRAM
  // ==================================================
  if (
    psramFound()
  ) {

    Serial.println(
      "PSRAM detected"
    );

    config.frame_size =
      FRAMESIZE_VGA;

    config.jpeg_quality =
      10;

    config.fb_count =
      2;

    config.grab_mode =
      CAMERA_GRAB_LATEST;
  }

  else {

    Serial.println(
      "PSRAM not detected"
    );

    config.frame_size =
      FRAMESIZE_QVGA;

    config.jpeg_quality =
      12;

    config.fb_count =
      1;
  }

  // ==================================================
  // CAMERA INIT
  // ==================================================
  esp_err_t err =
    esp_camera_init(
      &config
    );

  if (
    err != ESP_OK
  ) {

    Serial.printf(
      "Camera init failed: 0x%x\n",
      err
    );

    return;
  }

  Serial.println(
    "Camera initialized!"
  );

  // ==================================================
  // WIFI
  // ==================================================
  WiFi.begin(
    ssid,
    password
  );

  Serial.print(
    "Connecting to WiFi"
  );

  while (
    WiFi.status()
    != WL_CONNECTED
  ) {

    delay(
      500
    );

    Serial.print(
      "."
    );
  }

  Serial.println();

  Serial.println(
    "WiFi connected!"
  );

  // ==================================================
  // TIME WIB UTC+7
  // ==================================================
  configTime(
    7 * 3600,
    0,
    "pool.ntp.org",
    "time.nist.gov"
  );

  // ==================================================
  // START SERVER
  // ==================================================
  startCameraServer();

  // ==================================================
  // READY
  // ==================================================
  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "VisionNode V4 READY"
  );

  Serial.println(
    "Developed by Ratu Ramadhania"
  );

  Serial.println(
    "================================"
  );

  Serial.print(
    "Dashboard: http://"
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.print(
    "Stream: http://"
  );

  Serial.print(
    WiFi.localIP()
  );

  Serial.println(
    ":81/stream"
  );

  Serial.println(
    "================================"
  );
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  delay(
    1000
  );
}
