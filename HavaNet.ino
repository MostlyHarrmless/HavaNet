#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <MQ135.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>

//   WiFi configuration
const char* ssid = "Shahid Avini";
const char* password = "avini1111";

// Pins
#define SDA_PIN D14
#define SCL_PIN D15
#define DHTPIN D13
#define DHTTYPE DHT22
#define MQ135PIN A0 

// classes using
DHT dht(DHTPIN,DHTTYPE);
Adafruit_BMP280 bmp;
ESP8266WebServer server(80);
MQ135 gasSensor = MQ135(MQ135PIN);
 

// MODEL configuration


// Layer 1 

float weights1[8][3] = {
  { 0.28294972,  0.29153225,  0.449346   },
  { 0.31666666,  0.07020273,  0.34547874 },
  {-0.40061224,  0.28645352, -0.54273576 },
  {-0.62148523, -0.64696914, -0.56533444 },
  { 0.562767  ,  0.69121534,  0.6568565  },
  { 0.35976565,  0.00290278,  0.02970321 },
  { 0.30596152,  0.2857794 , -0.69261515 },
  { 0.5985757 , -0.30358952, -0.10468937 }
};


float bias1[8] = { 0.15216172, -0.11120524, -0.0198186, 0.0, 0.13501087, -0.05409049, -0.08679083, 0.0900683 };

// Layer 2


float weights2[4][8] = {
  { 0.5844913 , -0.24311408, -0.1902933 ,  0.31597155,  0.07713251, -0.56700987, -0.8208487 ,  0.06475028 },
  {-0.08247596, -0.29879013, -0.2262977 ,  0.11962205, -0.514657  , -0.2304433 ,  0.44778806, -0.11381739 },
  {-0.46136838, -0.34160176,  0.2535593 ,  0.6576075 , -0.28533706, -0.1464807 ,  0.41588098,  0.6623113  },
  {-0.37512448, -0.37719592, -0.34262505, -0.05846965, -0.15549117,  0.3552546 ,  0.2587071 , -0.23380998 }
};


float bias2[4] = { 0.15602383, 0.0, -0.0134545, 0.0 };

// Layer 3
float weights3[1][4] = {{0.7303473, -0.72093165, -0.7740962, -0.38549274}};

float bias3[1] = {0.15494013};

// Normalization Values

float x_min[3] = {26.5, 28.5, 49.15};
float x_max[3] = {29.4, 43.2, 122.62};
float y_min = -82.0;
float y_max = -49.0;


// Normalization Functions

void normalize_input(float* x) {
  for (int i = 0; i < 3; i++) {
    x[i] = (x[i] - x_min[i]) / (x_max[i] - x_min[i]);
    if (x[i] < 0) x[i] = 0;
    if (x[i] > 1) x[i] = 1;
  }
}

// ReLU

float relu(float x) {
  return x > 0 ? x : 0;
}

// Model

float predict_rssi(float* input) {
  normalize_input(input);

  float layer1[8];
  for (int i = 0; i < 8; i++) {
    layer1[i] = bias1[i];
    for (int j = 0; j < 3; j++) {
      layer1[i] += weights1[i][j] * input[j];
    }
    layer1[i] = relu(layer1[i]);
  }

  float layer2[4];
  for (int i = 0; i < 4; i++) {
    layer2[i] = bias2[i];
    for (int j = 0; j < 8; j++) {
      layer2[i] += weights2[i][j] * layer1[j];
    }
    layer2[i] = relu(layer2[i]);
  }


  float output = bias3[0];
  for (int i = 0; i < 4; i++) {
    output += weights3[0][i] * layer2[i];
  }

  return output * (y_max - y_min) + y_min;
}



void setup() {

  Serial.begin(115200);
  dht.begin();
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found!");
    while (1);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected. IP: " + WiFi.localIP().toString());
  if (!SPIFFS.begin()) {
    Serial.println("there is somethig wrong whith SPIFFS!");
    return;
}


server.on("/", []() {
  float temperatureFromDHT22 = dht.readTemperature();
  float temperatureFromBMP280 = bmp.readTemperature();
  float temperature = (temperatureFromBMP280 + temperatureFromDHT22) / 2;
  float pressure = bmp.readPressure() / 100.0;
  float humidity = dht.readHumidity();
  float gas = gasSensor.getPPM();
  long rssi = WiFi.RSSI();
  float input[3] = {temperature, humidity, gas};
  float predicted_rssi = predict_rssi(input);
  float error = rssi - predicted_rssi;
  String quality, icon, color;

  if (abs(error) <= 7) {
    quality = "کیفیت سیگنال خوب است";
    icon = "✅";
    color = "#c8e6c9";  // سبز
  } else if (abs(error) <= 15) {
    quality = "کیفیت سیگنال متوسط است";
    icon = "⚠️";
    color = "#fff9c4";  // زرد
  } else {
    quality = "کیفیت سیگنال ضعیف است";
    icon = "❌";
    color = "#ffcdd2";  // قرمز
  }

  int signalPercent = constrain(map(rssi, -100, -50, 0, 100), 0, 100);
  String statusDiv = "<div class='status' style='background-color: " + color + "; border-radius: 10px; padding: 20px; margin-top: 20px; font-weight: bold; font-size: 1.1em;'>"
                     + icon + " " + quality + "</div>";

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang='fa'>
  <head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <meta http-equiv='refresh' content='30'>
    <title>📊هوانت</title>
    <link href='https://fonts.googleapis.com/css2?family=Vazirmatn:wght@400;500;600&display=swap' rel='stylesheet'>
    <style>
      body {
        font-family: 'Vazirmatn', sans-serif;
        direction: rtl;
        background-color: #f1f1f1;
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
      }
      .container {
        background-color: #ffffff;
        border-radius: 16px;
        box-shadow: 0 8px 20px rgba(0, 0, 0, 0.1);
        padding: 40px;
        width: 90%;
        max-width: 600px;
        text-align: center;
      }
      h1 {
        font-size: 2.2em;
        color: #01579b;
        margin-bottom: 20px;
      }
      .data-section {
        display: flex;
        flex-direction: column;
        gap: 16px;
        margin-bottom: 20px;
      }
      .data-item {
        background: #fafafa;
        padding: 16px;
        border-radius: 12px;
        display: flex;
        justify-content: space-between;
        align-items: center;
        font-size: 1.1em;
        box-shadow: 0 4px 8px rgba(0,0,0,0.05);
      }
      .label {
        color: #37474f;
        font-weight: 500;
      }
      .value {
        font-weight: bold;
        color: #263238;
      }
      .icon {
        margin-left: 8px;
      }
      .progress-container {
        width: 100%;
        background-color: #eee;
        border-radius: 8px;
        overflow: hidden;
        margin-top: 8px;
        height: 14px;
      }
      .progress-bar {
        height: 100%;
        background-color: #4caf50;
        width: )rawliteral" + String(signalPercent) + R"rawliteral(%;
      }
      footer {
        margin-top: 30px;
        font-size: 0.85em;
        color: #78909c;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>📊هوانت </h1>
      <div class="data-section">
        <div class="data-item"><span class="label">🌡️ دما</span><span class="value">)rawliteral" + String(temperature, 1) + R"rawliteral( °C</span></div>
        <div class="data-item"><span class="label">💧 رطوبت</span><span class="value">)rawliteral" + String(humidity, 1) + R"rawliteral( ٪</span></div>
        <div class="data-item"><span class="label">🌬️ فشار</span><span class="value">)rawliteral" + String(pressure, 1) + R"rawliteral( hPa</span></div>
        <div class="data-item"><span class="label">🧪 گاز</span><span class="value">)rawliteral" + String(gas, 1) + R"rawliteral( ppm</span></div>
        <div class="data-item"><span class="label">📶 سیگنال</span><span class="value">)rawliteral" + String(rssi) + R"rawliteral( dBm</span></div>
        <div class="progress-container"><div class="progress-bar"></div></div>
        <div class="data-item"><span class="label">🤖 مقدار پیش‌بینی شده</span><span class="value">)rawliteral" + String(predicted_rssi, 1) + R"rawliteral( dBm</span></div>
        <div class="data-item"><span class="label">🔁 اختلاف</span><span class="value">)rawliteral" + String(error, 1) + R"rawliteral( dBm</span></div>
      </div>
  )rawliteral";

  html += statusDiv;

  html += R"rawliteral(
      <footer>آخرین بروزرسانی: <span id="time">در حال بارگذاری...</span></footer>
    </div>
    <script>
      const now = new Date();
      const timeString = now.toLocaleTimeString('fa-IR');
      document.getElementById('time').textContent = timeString;
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
});




  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();   
}
