#include <WiFi.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ135PIN 34  

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Saketh";
const char* password = "********";

WiFiServer server(80);

void connectToWiFi() {
  Serial.printf("Connecting to WiFi SSID: %s\n", ssid);
  WiFi.begin(ssid, password);

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {
    delay(1000);
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

const char* interpretAirQuality(int raw) {
  if (raw < 150) return "Excellent";
  else if (raw < 300) return "Good";
  else if (raw < 450) return "Fair";
  else if (raw < 600) return "Poor";
  else return "Very Poor";
}

void handleClient(WiFiClient &client) {
  Serial.println("New client connected");

  String request = "";
  unsigned long timeout = millis() + 2000;

  while (client.connected() && millis() < timeout) {
    while (client.available()) {
      char c = client.read();
      request += c;
      if (request.endsWith("\r\n\r\n")) {
        timeout = 0;
        break;
      }
    }
    delay(1);
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int mq135Raw = analogRead(MQ135PIN);
  float mq135Voltage = mq135Raw * (3.3 / 4095.0);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    humidity = 0;
    temperature = 0;
  }

  Serial.printf("Temperature: %.1f °C, Humidity: %.1f %%\n", temperature, humidity);
  Serial.printf("MQ-135 Raw: %d, Voltage: %.2f V\n", mq135Raw, mq135Voltage);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html; charset=UTF-8");
  client.println("Connection: close");
  client.println();

  client.println(R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta http-equiv="refresh" content="5" />
<title>ESP32 Sensor Dashboard</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600&display=swap');
  body {
    font-family: 'Inter', sans-serif;
    background: #f5f7fa;
    color: #222;
    margin: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: flex-start;
    min-height: 100vh;
    padding: 40px 20px;
  }
  h1 {
    font-weight: 600;
    color: #0d3b66;
    margin-bottom: 40px;
  }
  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
    gap: 48px 60px;
    width: 100%;
    max-width: 900px;
  }
  .card {
    background: #fff;
    border-radius: 14px;
    padding: 28px 32px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    align-items: center;
    border: 2px solid #e1e8f0;
  }
  .label {
    font-weight: 600;
    font-size: 1.1rem;
    margin-bottom: 14px;
    color: #344055;
  }
  .value {
    font-size: 2rem;
    font-weight: 700;
    color: #1d3557;
  }
  .interpretation {
    margin-top: 12px;
    font-size: 1rem;
    font-weight: 600;
    color: #4a90e2;
  }
  footer {
    margin-top: 60px;
    font-size: 0.9rem;
    color: #777;
  }
</style>
</head>
<body>
  <h1>ESP32 Sensor Dashboard</h1>
  <div class="grid">
    <div class="card">
      <div class="label">Temperature</div>
      <div class="value">)rawliteral");

  client.printf("%.1f &deg;C", temperature);

  client.println(R"rawliteral(
      </div>
    </div>
    <div class="card">
      <div class="label">Humidity</div>
      <div class="value">)rawliteral");

  client.printf("%.1f %%", humidity);

  client.println(R"rawliteral(
      </div>
    </div>
    <div class="card">
      <div class="label">Air Quality (MQ-135)</div>
      <div class="value">)rawliteral");

  client.printf("%d (Raw), %.2f V", mq135Raw, mq135Voltage);

  client.println(R"rawliteral(
      </div>
      <div class="interpretation">)rawliteral");

  client.print(interpretAirQuality(mq135Raw));

  client.println(R"rawliteral(
      </div>
    </div>
  </div>
  <footer>Page refreshes every 5 seconds</footer>
</body>
</html>
)rawliteral");

  delay(1);
  client.stop();
  Serial.println("Client disconnected");
}

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing sensors...");
  dht.begin();
  delay(2000); // Allow sensors to stabilize

  float initHumidity = dht.readHumidity();
  float initTemp = dht.readTemperature();
  if (!isnan(initHumidity) && !isnan(initTemp)) {
    Serial.printf("Initial Temperature: %.1f °C\n", initTemp);
    Serial.printf("Initial Humidity: %.1f %%\n", initHumidity);
  } else {
    Serial.println("Initial sensor read failed.");
  }

  connectToWiFi();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}
