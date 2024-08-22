#include <Adafruit_AHTX0.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include "ThingSpeak.h"
#include <PubSubClient.h>

#define fan_pin 25
#define pomp_pin 26
#define sensor_pin 32

Adafruit_AHTX0 aht;

WiFiClient client1;

// MQTT Broker
const char *mqtt_broker = "maqiatto.com";
const char *topic1 = "muhammadnabiyevolloyor@gmail.com/set_room_humidity";
const char *topic2 = "muhammadnabiyevolloyor@gmail.com/set_solid_humidity";
const char *mqtt_username = "muhammadnabiyevolloyor@gmail.com";
const char *mqtt_password = "student";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

int myChannelNumber = 1;
const char *myWriteAPIKey = "7LD7OGXORCH5H4A7";
unsigned long ttime = 110000;
byte normal_hum = 50;
int normal_hum_sol = 2000;
int solidHum;
String myStatus = "OK";
String buffer;
bool humFlag = 0;
bool humFlagSolid = 0;

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  buffer = "";
  for (int i = 0; i < length; i++)
  {
    buffer += (char)payload[i];
  }
  if (strcmp(topic, topic1) == 0)
  {
    Serial.println(topic);
    normal_hum = buffer.toInt();
  }
  if (strcmp(topic, topic2) == 0)
  {
    Serial.println(topic);
    normal_hum_sol = buffer.toInt();
  }
  Serial.print("Message:");
  Serial.println(buffer);
  Serial.println();
  Serial.println("-----------------------");
}

void setup()
{
  pinMode(sensor_pin, INPUT);
  pinMode(fan_pin, OUTPUT);
  pinMode(pomp_pin, OUTPUT);

  Serial.begin(115200);
  Serial.println("Adafruit AHT10/AHT20 demo!");

  if (!aht.begin())
  {
    Serial.println("Could not find AHT? Check wiring");
    while (1)
      delay(10);
  }
  Serial.println("AHT10 or AHT20 found");

  WiFiManager wifiManager;

  if (!wifiManager.autoConnect("esp32", "password"))
  {
    Serial.println("failed to connect and hit timeout");
    // reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client1); // Initialize ThingSpeak

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public EMQX MQTT broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // Publish and subscribe
  client.subscribe(topic1);
  client.subscribe(topic2);
}

void loop()
{
  client.loop();
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  solidHum = analogRead(sensor_pin);
  if (millis() - ttime >= 120000)
  {
    ttime = millis();
    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degrees C");
    Serial.print("Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println("% rH");
    Serial.print("solid Humidity: ");
    Serial.print(solidHum);
    Serial.println("rH");

    // set the fields with the values
    ThingSpeak.setField(1, temp.temperature);
    ThingSpeak.setField(2, humidity.relative_humidity);
    ThingSpeak.setField(3, solidHum);

    // set the status
    ThingSpeak.setStatus(myStatus);
// write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200)
    {
      Serial.println("Channel update successful.");
    }
    else
    {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  }
  if (!humFlag and humidity.relative_humidity > normal_hum)
  {
    humFlag = 1;
    digitalWrite(pomp_pin, 1);
  }
  if (humFlag and humidity.relative_humidity <= (normal_hum - 2))
  {
    humFlag = 0;
    digitalWrite(pomp_pin, 0);
  }

  if (!humFlagSolid and solidHum > normal_hum_sol)
  {
    humFlagSolid = 1;
  }
  if (humFlagSolid and solidHum <= (normal_hum_sol - 10))
  {
    humFlagSolid = 0;
  }
  if (humFlagSolid)
  {
    digitalWrite(fan_pin, 1);
    delay(150);
    digitalWrite(fan_pin, 0);
    delay(50);
  }
}
