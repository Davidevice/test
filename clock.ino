//json versione 5.13  VERSIONE SCHEDA WEMOS
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Arduino.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include "CTBot.h"       //https://github.com/Di-Strix/CTBot   modificata dalla https://github.com/shurillu/CTBot
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   D2
#define DATA_PIN  D4
#define CS_PIN    D3
#include <NTPClient.h>
int Offset = 7200;   //3600 = 1h
int ctrlMessaggio = 0;
int ctrlUScita = 0;
char* newMessageOra;
String strDaOra;
String payload;
bool oraTime;
//long interval  = 10000;
//long previousMillis = 0;
long current;
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.it.pool.ntp.org", Offset, 60000);
//(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
uint8_t scrollSpeed = 25;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 0; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  512

char char_array;
char curMessage[BUF_SIZE] = { " o_O   +_+    *-*   :_:  " };
char newMessage[BUF_SIZE] = { " MANDA UN MESSAGGIO SU TELEGRAM " };
bool newMessageAvailable = true;
CTBot myBot;
CTBotInlineKeyboard myKbd;
WiFiClient client;
#define HOSTNAME "Telegram BOt"
String token = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

long previousMillis = 0;
long interval = 10000;

void setup() {
  Serial.begin(9600);
  WiFiManager wifiManager;
  wifiManager.autoConnect("Telegram BOt TEST");

  //oraTime = false;

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

  timeClient.begin();
  //  myKbd.addButton("OFF", LIGHT_OFF_CALLBACK, CTBotKeyboardButtonQuery);


  // set the telegram bot token
  myBot.setTelegramToken(token);

  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("TEST CTBOT OK");
  else
    Serial.println("TEST CTBO KO");
  P.begin();
  P.setIntensity(1);
  //P.setInvert(true);

  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);


}

void loop() {
  ArduinoOTA.handle();




  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage); Serial.println("eccolo");
      newMessageAvailable = false;
    }
    P.displayReset();
    Serial.println("eccomi qui");
    Serial.print(newMessage);                                                        //////////
    P.displayText(newMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);    /////////
    P.displayAnimate();                                                                               ///////
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      Serial.println("sto avv messaggio");
      messaggio();

    }

  }
}
void ora()  {
  //while (oraTime == true){

  while (oraTime == true) {
    TBMessage msg;
    timeClient.update();
    String oraStr = (timeClient.getFormattedTimeNoSeconds());
    P.print(" " + (oraStr));
    delay(5000);

    int a = ((myBot.getNewMessage(msg)));
    strDaOra = msg.text;//Serial.print(" a   "); Serial.println((myBot.getNewMessage(msg)));


    if (a == 1) {
      myBot.sendMessage(msg.sender.id, "Esco da orologio");   /////////////////////////////////////////

      TBMessage msg; ////////////////////////////////////////////////
      Serial.println(" esco da orologio");
      oraTime = false;
      a = 0;
      ctrlMessaggio = 1;
      newMessageAvailable = true;
      //return;// messaggio();// newMessageAvailable = true; ///////////////////
      Serial.print("str in ora    "); Serial.println(strDaOra);
    messaggio();
    }

  }

}

void messaggio() {
  Serial.print("ora time    "); Serial.println(oraTime);

  if (oraTime == false) { //if (oraTime == false){


    if (ctrlMessaggio == 1) {
      Serial.println("ctrl messaggio     ");
      Serial.print("str in messaggio    "); Serial.println(strDaOra);
      ctrlMessaggio = 0;
      int str_len = strDaOra.length() + 1;
      char char_array[str_len];

      strDaOra.toCharArray(char_array, str_len);

      strcpy (newMessage, char_array);

      newMessageAvailable = true;
      ctrlUScita = 1;
      return;
    }


    TBMessage msg;
    if (myBot.getNewMessage(msg)) {

      if (msg.text.equalsIgnoreCase("ora")) {
        myBot.sendMessage(msg.sender.id, "Attivo orologio, digita qualcosa per uscire e mostrare il messaggio!");
        oraTime = true; ora();
        // notify the sender
      }

      if (msg.text.equalsIgnoreCase("TEST")) {
        myBot.sendMessage(msg.sender.id, "TEST OK");
       return;
       }
     
      if (msg.text.equalsIgnoreCase("aiuto")) {
        myBot.sendMessage(msg.sender.id, "Scrivi 'Ora' per mostrare l'orologio.Dall'orologio digita cio' che vuoi e verra' mostrato sul display.");
      return;
      }

   if (ctrlUScita == 0){   
      Serial.print("ctr scita zero"); //Serial.println(str);
      //myBot.getNewMessage(msg); ////// aggiunto alle 1838
      String str = msg.text; Serial.print("MSG:TEXT"); Serial.println(str);
      int str_len = str.length() + 1;
      char char_array[str_len];

      str.toCharArray(char_array, str_len);

      strcpy (newMessage, char_array);

      newMessageAvailable = true;       // aggiunto l else
      Serial.println("sono alla fine del messaggio() " );
      }

if (ctrlUScita == 1){   
        Serial.print("ctr scita uno"); Serial.println(strDaOra);
      //myBot.getNewMessage(msg); ////// aggiunto alle 1838
     // String strDaOra = msg.text; 
      Serial.print("MSG:TEXT due"); Serial.println(strDaOra);
      int str_len = strDaOra.length() + 1;
      char char_array[str_len];

      strDaOra.toCharArray(char_array, str_len);

      strcpy (newMessage, char_array);

      newMessageAvailable = true;       // aggiunto l else
      Serial.println("sono alla fine del messaggio() " );}




      
    }
/// forse qua devi mettere 
ctrlUScita = 0;
  }


  if (oraTime == true) {
    ora();
    Serial.println("vado in ora");
  }


}
