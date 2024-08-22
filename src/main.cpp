#include <Arduino.h>
#include <WiFi.h>

const char* ssid = "ProRect";
const char* password = "123456789";

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<!DOCTYPE html><html>");
            client.println("<head><title>ESP32 Web Server</title></head>");
            client.println("<body><h1>Hello World</h1><button>on</button>");
            client.println("<button>off</button> <a href=\"./on\">Turn on</a>");
            client.println("<a href=\"./off\">Turn off</a></body>");
            client.println("</html>");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}
