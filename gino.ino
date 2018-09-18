
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Arduino.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
WiFiClient client;

int notifyLed = D1;
int checkNotify = D2;
int day, month, year, dayOfWeek;
int summerTime = 0;
String date;
int stateCheck = LOW;

#define NUM_MAX 4
#define DIN_PIN D7  // D7
#define CS_PIN  D8  // D3
#define CLK_PIN D5  // D5

#include "max7219.h"
#include "Fonts.h"
#define HOSTNAME "ESP8266-OTA-"

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "davidevice"
#define AIO_KEY         "31a0e3ec7bd04636a485c05bd28b55b4"

const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

char* str;
String payload;

//uint32_t present;
//bool first_time;

const char* ssid     = "ASUS";     // SSID of local network
const char* password = "2325051316";   // Password on network

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Subscribe message = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/message");
Adafruit_MQTT_Subscribe lifx = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/lifx");
Adafruit_MQTT_Subscribe photocell = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/photocell");

void setup()
{


  Serial.begin(115200);
  pinMode(notifyLed, OUTPUT);
  pinMode(checkNotify, INPUT);
  initMAX7219();
  sendCmdAll(CMD_SHUTDOWN, 1);
  sendCmdAll(CMD_INTENSITY, 2);
  Serial.print("Connecting WiFi ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  printStringWithShift("Connecting", 15);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("MyIP: "); Serial.println(WiFi.localIP());
  printStringWithShift((String("  MyIP: ") + WiFi.localIP().toString()).c_str(), 15);
  delay(1500);
  mqtt.subscribe(&photocell);
  mqtt.subscribe(&lifx);
  mqtt.subscribe(&message);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


// =============================DEFINE VARS==============================
#define MAX_DIGITS 20
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx = 0;
int dy = 0;
byte del = 0;
int h, m, s;
float utcOffset = +2;
long localEpoc = 0;
long localMillisAtUpdate = 0;
// =======================================================================



void loop()
{

  ArduinoOTA.handle();

  MQTT_connect();
  updateTime();
  showAnimClock();

  if (updCnt <= 0) { // every 10 scrolls, ~450s=7.5m
    mqtt.ping(); /////////////////////////////////////////////////////////////ping
    updCnt = 60;
    Serial.println("Getting data ...");
    printStringWithShift("   Setting Time...", 15);
    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }

  if (millis() - clkTime > 60000 && !del && dots) { // clock for 30s, then scrolls for about 30s
    //    printStringWithShift(date.c_str(),40);
    //    delay(2000);
    updCnt--;
    clkTime = millis();
  }

  if (millis() - dotTime > 500) {
    dotTime = millis();
    dots = !dots;
  }

  Adafruit_MQTT_Subscribe *subscription;

  stateCheck = digitalRead(checkNotify);

  if (stateCheck == HIGH) {                               ////////////////////////così ok ma devi tenere sempre il pulsante LOW con resistenza
    check();
  }

  while ((subscription = mqtt.readSubscription(1)))


  {
    if (! mqtt.ping()) {
      mqtt.connected();
    }
    if (subscription == &message) {
      digitalWrite(notifyLed, HIGH);
      payload = "";
      Serial.print(F("Got: "));
      Serial.println((char *)message.lastread);
      str = (char*)message.lastread;
      payload = (String) str;
      payload += "      ";
      str = &payload[0];
      for (int i = 0; i < 3; i++) {
        printStringWithShift(str, 54);
      }

    }


    if (subscription == &lifx) {

      if (strcmp((char *)lifx.lastread, "MAIL") == 0) {
        Serial.println("Arrivata una mail");
        mail();
      }
      if (strcmp((char *)lifx.lastread, "MESS") == 0) {
        Serial.println("ArrivatO MESSAGGIO");
        messaggio();
      }
      if (strcmp((char *)lifx.lastread, "myip") == 0) {
        Serial.println("ip");
        printStringWithShift((String("  MyIP: ") + WiFi.localIP().toString()).c_str(), 15);
        delay(1500);
      }
    }
  }
}


void mail() {
  refreshAll();
  for (int i = 0; i < 6; i++) {
    printStringWithShift("E' arrivata una mail   ", 43);
  }
}

void messaggio() {
  for (int i = 0; i < 6; i++) {
    printStringWithShift("E' arrivato un messaggio giovane!!   ", 43);
  }
}

void check() {            //this repeat the last message received from google "scrivi ...."
  for (int i = 0; i < 6; i++) {
    printStringWithShift(str, 43);
    digitalWrite(notifyLed, 0);
  }
}

void showSimpleClock()
{
  dx = dy = 0;
  clr();
  showDigit(h / 10,  0, dig6x8);
  showDigit(h % 10,  8, dig6x8);
  showDigit(m / 10, 17, dig6x8);
  showDigit(m % 10, 25, dig6x8);
  showDigit(s / 10, 34, dig6x8);
  showDigit(s % 10, 42, dig6x8);
  setCol(15, dots ? B00100100 : 0);
  setCol(32, dots ? B00100100 : 0);
  refreshAll();
}

// =======================================================================

void showAnimClock() {



  byte digPos[6] = {0, 8, 17, 25, 34, 42};
  int digHt = 12;
  int num = 6;
  int i;
  if (del == 0) {
    del = digHt;
    for (i = 0; i < num; i++) digold[i] = dig[i];
    dig[0] = h / 10 ? h / 10 : 10;
    dig[1] = h % 10;
    dig[2] = m / 10;
    dig[3] = m % 10;
    dig[4] = s / 10;
    dig[5] = s % 10;
    for (i = 0; i < num; i++)  digtrans[i] = (dig[i] == digold[i]) ? 0 : digHt;
  } else
    del--;

  clr();
  for (i = 0; i < num; i++) {
    if (digtrans[i] == 0) {
      dy = 0;
      showDigit(dig[i], digPos[i], dig6x8);
    } else {
      dy = digHt - digtrans[i];
      showDigit(digold[i], digPos[i], dig6x8);
      dy = -digtrans[i];
      showDigit(dig[i], digPos[i], dig6x8);
      digtrans[i]--;
    }
  }

  dy = 0;
  setCol(15, dots ? B00100100 : 0);
  setCol(32, dots ? B00100100 : 0);
  refreshAll();
  delay(30);

}


// =======================================================================

void showDigit(char ch, int col, const uint8_t *data)
{
  if (dy < -8 | dy > 8) return;
  int len = pgm_read_byte(data);
  int w = pgm_read_byte(data + 1 + ch * len);
  col += dx;
  for (int i = 0; i < w; i++)
    if (col + i >= 0 && col + i < 8 * NUM_MAX) {
      byte v = pgm_read_byte(data + 1 + ch * len + 1 + i);
      if (!dy) scr[col + i] = v; else scr[col + i] |= dy > 0 ? v >> dy : v << -dy;
    }

}

// =======================================================================

void setCol(int col, byte v)
{
  if (dy < -8 | dy > 8) return;
  col += dx;
  if (col >= 0 && col < 8 * NUM_MAX)
      if (!dy) scr[col] = v; else scr[col] |= dy > 0 ? v >> dy : v << -dy;
}

// =======================================================================

int showChar(char ch, const uint8_t *data)
{
  int len = pgm_read_byte(data);
  int i, w = pgm_read_byte(data + 1 + ch * len);
  for (i = 0; i < w; i++)
    scr[NUM_MAX * 8 + i] = pgm_read_byte(data + 1 + ch * len + 1 + i);
  scr[NUM_MAX * 8 + i] = 0;
  return w;
}

// =======================================================================
int dualChar = 0;

unsigned char convertPolish(unsigned char _c)
{
  unsigned char c = _c;
  if (c == 196 || c == 197 || c == 195) {
    dualChar = c;
    return 0;
  }
  if (dualChar) {
    switch (_c) {
      case 133: c = 1 + '~'; break; // 'ą'
      case 135: c = 2 + '~'; break; // 'ć'
      case 153: c = 3 + '~'; break; // 'ę'
      case 130: c = 4 + '~'; break; // 'ł'
      case 132: c = dualChar == 197 ? 5 + '~' : 10 + '~'; break; // 'ń' and 'Ą'
      case 179: c = 6 + '~'; break; // 'ó'
      case 155: c = 7 + '~'; break; // 'ś'
      case 186: c = 8 + '~'; break; // 'ź'
      case 188: c = 9 + '~'; break; // 'ż'
      //case 132: c = 10+'~'; break; // 'Ą'
      case 134: c = 11 + '~'; break; // 'Ć'
      case 152: c = 12 + '~'; break; // 'Ę'
      case 129: c = 13 + '~'; break; // 'Ł'
      case 131: c = 14 + '~'; break; // 'Ń'
      case 147: c = 15 + '~'; break; // 'Ó'
      case 154: c = 16 + '~'; break; // 'Ś'
      case 185: c = 17 + '~'; break; // 'Ź'
      case 187: c = 18 + '~'; break; // 'Ż'
      default:  break;
    }
    dualChar = 0;
    return c;
  }
  switch (_c) {
    case 185: c = 1 + '~'; break;
    case 230: c = 2 + '~'; break;
    case 234: c = 3 + '~'; break;
    case 179: c = 4 + '~'; break;
    case 241: c = 5 + '~'; break;
    case 243: c = 6 + '~'; break;
    case 156: c = 7 + '~'; break;
    case 159: c = 8 + '~'; break;
    case 191: c = 9 + '~'; break;
    case 165: c = 10 + '~'; break;
    case 198: c = 11 + '~'; break;
    case 202: c = 12 + '~'; break;
    case 163: c = 13 + '~'; break;
    case 209: c = 14 + '~'; break;
    case 211: c = 15 + '~'; break;
    case 140: c = 16 + '~'; break;
    case 143: c = 17 + '~'; break;
    case 175: c = 18 + '~'; break;
    default:  break;
  }
  return c;
}

// =======================================================================

void printCharWithShift(unsigned char c, int shiftDelay) {
  c = convertPolish(c);
  if (c < ' ' || c > '~' + 25) return;
  c -= 32;
  int w = showChar(c, font);
  for (int i = 0; i < w + 1; i++) {
    delay(shiftDelay);
    scrollLeft();
    refreshAll();
  }
}

// =======================================================================




void printStringWithShift(const char* s, int shiftDelay) {
  while (*s) {
    printCharWithShift(*s, shiftDelay);
    s++;
  }
}
// =======================================================================

void getTime()
{



  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    //Serial.println(".");
    repeatCounter++;
  }

  String line;
  client.setNoDelay(false);
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      date = "     " + line.substring(6, 22);
      date.toUpperCase();
      //      decodeDate(date);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      summerTime = checkSummerTime();
      if (h + utcOffset + summerTime > 23) {
        if (++day > 31) {
          day = 1;
          month++;
        };  // needs better patch
        if (++dayOfWeek > 7) dayOfWeek = 1;
      }
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }


  client.stop();
}

// =======================================================================

int checkSummerTime()
{
  if (month > 3 && month < 10) return 1;
  if (month == 3 && day >= 31 - (((5 * year / 4) + 4) % 7) ) return 1;
  if (month == 10 && day < 31 - (((5 * year / 4) + 1) % 7) ) return 1;
  return 0;
}
// =======================================================================

// =======================================================================

void updateTime()
{


  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * (utcOffset + summerTime) + 86400L) % 86400L;
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");


  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds

  }


  //updateTime();
  //showAnimClock();
  Serial.println("MQTT Connected!");
}

// =======================================================================

