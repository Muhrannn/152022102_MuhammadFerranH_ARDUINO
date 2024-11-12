#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 18
#define DHTTYPE DHT11
#define LED_GREEN 5
#define LED_YELLOW 10
#define LED_RED 12
#define RELAY_PUMP 7
#define BUZZER 9

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "Wokwi-GUEST";            
const char* password = "";  
const char* mqtt_server = "broker.hivemq.com";   
const char* mqtt_sub_topic = "test1";  
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung.");
}

void GetMessage(char* topic, byte* message, unsigned int length) {
  Serial.print("Pesan diterima pada topic: ");
  Serial.println(topic);

  Serial.print("Pesan: ");
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)message[i];
  }
  Serial.println(msg);

  if (client.publish(mqtt_sub_topic, msg.c_str())) {
    Serial.println("Pesan berhasil diteruskan ke MQTTBox.");
  } else {
    Serial.println("Gagal meneruskan pesan ke MQTTBox.");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP32Client")) {  
      Serial.println("Terhubung.");
      client.subscribe(mqtt_sub_topic);  // Subscribe ke topic test1
      Serial.println("Subscribed to topic: test1");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" Coba lagi dalam 5 detik.");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);

  dht.begin();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(GetMessage); // Set fungsi callback
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca data dari DHT sensor!");
    return;
  }

  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print("°C Kelembapan: ");
  Serial.print(kelembapan);
  Serial.println("%");

  if (suhu > 35) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(RELAY_PUMP, HIGH);  
  } else if (suhu >= 30 && suhu <= 35) {
    digitalWrite(LED_YELLOW, HIGH);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER, LOW);
    digitalWrite(RELAY_PUMP, LOW);  
  } else {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER, LOW);
    digitalWrite(RELAY_PUMP, LOW);  
  }

  char payload[50];
  snprintf(payload, sizeof(payload), "Suhu: %.2f°C, Kelembapan: %.2f%%", suhu, kelembapan);
  if (client.publish(mqtt_sub_topic, payload)) {
    Serial.println("Data terkirim ke MQTT:");
    Serial.println(payload);
  } else {
    Serial.println("Gagal mengirim data ke MQTT.");
  }
  delay(2000);
}