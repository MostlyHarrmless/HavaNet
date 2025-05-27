# 🌤️ HavaNet — Smart Weather Logger with TinyML

HavaNet is an intelligent weather monitoring system built using **Wemos D1 mini**, a set of environmental sensors, and a lightweight machine learning model that predicts signal strength (RSSI) based on real-time temperature, humidity, and air quality.

Designed for embedded applications, **HavaNet** logs data locally, visualizes it with a Persian-styled HTML interface, and even runs a manually implemented ML model *without any external library*.

---

## 🚀 Features

- 🌡️ Measures **temperature**, **humidity**, **pressure**, and **gas concentration**
- 📶 Predicts **RSSI** using a hand-implemented ML model
- 📊 Displays data on a beautiful, responsive **HTML dashboard** with auto-refresh
- 💾 Logs environmental data for future use and visualization
- 🌐 Runs on **Wemos D1 Mini** (ESP8266) – no internet required

---

## 🔧 Hardware Used

| Component        | Description                    |
|------------------|--------------------------------|
| Wemos D1 Mini    | ESP8266 Wi-Fi Microcontroller  |
| DHT22            | Temp & Humidity Sensor         |
| BMP180           | Pressure Sensor                |
| MQ135            | Air Quality Sensor             |

---

## 🧠 Machine Learning Model

An **Artificial Neural Networks** is used to estimate the RSSI from input environmental values. The model is trained offline and deployed as raw C code with direct application of weights and bias — no ML libraries used!


