/****************************************************************************
 * Doorbell using a current sensor on the front door only,
 * sending notifications via ntfy.sh.
 ****************************************************************************/
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// ----------- USER CONFIGURATIONS ---------------
const char* ssid       = "your-ssid";
const char* password   = "your-wpa-passwd";
// Replace with your own ntfy.sh topic or URL
const char* ntfyUrl    = "https://ntfy.sh/87a7e5bf1204cb1e8f7bbac461722fb0";
// ------------------------------------------------

// The front doorbell is attached to analog pin A7 (GPIO35 on ESP32).
#define front_doorbell A7

// Debounce in milliseconds (so we don't spam notifications on noisy signals)
#define debounce 1000

// The ADC threshold to decide if the doorbell is “pressed”
#define bell_threshold 50

// Track the last time we sent a notification
unsigned long prevRing = 0;

// Simple logging utility to prepend a timestamp
void log_msg(const String &msg)
{
  time_t now = time(nullptr);
  String tm = ctime(&now);
  tm.trim();
  Serial.print(tm);
  Serial.print(": ");
  Serial.println(msg);
}

// Helper function to send a notification via ntfy.sh
void ring_bell(String bell, int senseDoorbell)
{
  // For debugging
  log_msg(bell + " doorbell pressed! ADC=" + String(senseDoorbell));

  // Prepare the payload for ntfy.sh
  // For example, we can pass some text describing the event
  String payload = bell + " doorbell pressed! ADC=" + String(senseDoorbell);

  // Use the WiFiClient with HTTPClient to make a POST request
  WiFiClient wifiClient;
  HTTPClient http;
  http.begin(wifiClient, ntfyUrl);
  http.addHeader("Content-Type", "text/plain"); // We'll send plain text
  int httpResponseCode = http.POST(payload);

  // Log the result
  if (httpResponseCode > 0) {
    log_msg("ntfy.sh response code: " + String(httpResponseCode));
  } else {
    log_msg("ntfy.sh request failed, error: " + http.errorToString(httpResponseCode));
  }
  http.end();
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(++i); Serial.print(' ');
    if (i > 10 ) {
      Serial.println(" WiFi connection failed; rebooting...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Get the current time (for timestamp logs); optional but useful for debugging
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(1000);

  // Setup the doorbell pin
  pinMode(front_doorbell, INPUT);
}

void loop()
{
  // Only check the doorbell if enough time has passed since last press
  if (millis() - prevRing >= debounce)
  {
    int senseDoorbell = analogRead(front_doorbell);
    if (senseDoorbell > bell_threshold)
    {
      ring_bell("Front", senseDoorbell);
      prevRing = millis();
    }
  }

  // (Optional) put other code here, e.g., for OTA updates, etc.
}
