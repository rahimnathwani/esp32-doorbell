#include <WiFi.h>
#include <HTTPClient.h>
#include "network_conn.h" // This should contain your WiFi credentials

#define back_doorbell A6
#define front_doorbell A7

#define debounce 1000
#define bell_threshold 50

// For ntfy.sh
const char *ntfyUrlBack = "https://ntfy.sh/mytopic/back_doorbell";
const char *ntfyUrlFront = "https://ntfy.sh/mytopic/front_doorbell";

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) // Wait for the Wi-Fi to connect
  {
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
    // Sometimes the ESP32 just fails to connect!  Reboot if unsuccessful
    if (i > 10)
    {
      Serial.print("Give up; reboot!");
      ESP.restart();
    }
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname:\t");
  Serial.println(WiFi.getHostname());

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(1000);

  pinMode(back_doorbell, INPUT);
  pinMode(front_doorbell, INPUT);

#ifdef NETWORK_UPDATE
  __setup_updater();
#endif
}

void ring_bell(String bell, const char *ntfyUrl, int senseDoorbell)
{
  HTTPClient http;
  http.begin(ntfyUrl); // Specify the URL
  http.addHeader("Content-Type", "text/plain");
  String payload = bell + " Ding Dong -- " + String(senseDoorbell);
  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0)
  {
    String response = http.getString(); // Get the response to the request
    Serial.println(httpResponseCode);   // Print return code
    Serial.println(response);           // Print request answer
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end(); // Free resources
}

int senseDoorbell = 0;
unsigned long prevRing = 0;

void loop()
{
  // Only check the doorbell if it hasn't been hit in the last second
  if (millis() - prevRing >= debounce)
  {
    senseDoorbell = analogRead(back_doorbell);

    if (senseDoorbell > bell_threshold)
    {
      ring_bell("Back", ntfyUrlBack, senseDoorbell);
      prevRing = millis();
    }

    senseDoorbell = analogRead(front_doorbell);
    if (senseDoorbell > bell_threshold)
    {
      ring_bell("Front", ntfyUrlFront, senseDoorbell);
      prevRing = millis();
    }
  }

#ifdef NETWORK_UPDATE
  __netupdateServer.handleClient();
#endif
}
