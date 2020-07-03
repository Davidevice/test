//json versione 5.13  VERSIONE SCHEDA WEMOS
// questo va, madonna se va!!!
#include <NTPClient.h>
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



int Offset = 7200;   //3600 = 1h
int ctrlMessaggio = 0;
int ctrlUScita = 0;   // serve per ignorare il messaggio mandato dal pulsante tastiera
char* newMessageOra;
String strDaOra;
String payload;
bool oraTime;
long current;


MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "0.it.pool.ntp.org", Offset, 60000);

//VARIABILI DISPLAY

uint8_t scrollSpeed = 23;
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 0;


#define  BUF_SIZE  1024

char char_array;
char curMessage[BUF_SIZE] = { "  o_O    +_+    *-*  " };
char newMessage[BUF_SIZE] = { " MANDA UN MESSAGGIO SU TELEGRAM " };
bool newMessageAvailable = true;

#define SIMPLE_CALLBACK  "Simple"

CTBot myBot;
CTBotReplyKeyboard myKbd;
bool isKeyboardActive;
WiFiClient client;
#define HOSTNAME "Telegram BOt"
String token = "1180054157:AAHSy0hngOFOborwhvLVmkLBVNgJXjPH--8";

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

  myBot.setTelegramToken(token);

  if (myBot.testConnection())
    Serial.println("TEST CTBOT OK");
  else
    Serial.println("TEST CTBO KO");



  myKbd.addButton("\U0001F551 Orologio");
  myKbd.addButton("\U00002753 Stato");
  // myKbd.addButton("Location request", CTBotKeyboardButtonLocation);
  myKbd.addRow();
  // add a button that send a message with "Hide replyKeyboard" text
  // (it will be used to hide the reply keyboard)
  myKbd.addButton("\U0001F6E0 - Officina Vicenzi -");
  // resize the keyboard to fit only the needed space
  myKbd.enableResize();
  isKeyboardActive = false;




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
    Serial.print(newMessage);
    P.displayText(newMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
    P.displayAnimate();
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;
      Serial.println("sto avviando messaggio  ");
      messaggio();

    }

  }
}
void ora()  {
  Serial.print(" ora time in ora()  "); Serial.println(oraTime);
  TBMessage msg;
  oraTime = true;  //////////////////////////////////////////////////////////////////////////////////////
  while (oraTime == true) {
    // TBMessage msg;  lo sposto allinizio
    timeClient.update();
    String oraStr = (timeClient.getFormattedTimeNoSeconds());
    P.print(" " + (oraStr));
    delay(5000);

    int a = ((myBot.getNewMessage(msg)));  // se c'è un messaggio a == 1
    strDaOra = msg.text; 


    if (a == 1) {


      if (msg.text.equalsIgnoreCase("ud83dudee0 - Officina Vicenzi -")) {
        myBot.sendMessage(msg.sender.id, "sta scorrendo il messaggio predefinito!");
        myBot.sendMessage(msg.sender.id, "\U0001F6E0 'OFFICINA VICENZI'");
        ctrlUScita = 1;
        strDaOra = "-- OFFICINA VICENZI --";
      }
      if (msg.text.equalsIgnoreCase("u2753 Stato")) {   ///era else if
        // Serial.print(" ora time"); Serial.println(oraTime);

        myBot.sendMessage(msg.sender.id, (F("Sto visualizzando l'orologio. ")));
        myBot.sendMessage(msg.sender.id, "A proposito ... sono le : " + String(oraStr));
        msg.text = "";
        oraTime = true;
        ora;
      }
 else     if (msg.text.equalsIgnoreCase("ud83dudd51 Orologio")) {
        myBot.sendMessage(msg.sender.id, (F("Sei gia' nell'orologio")));
        msg.text = "";
        oraTime = true;
        ora;
      }

      else {
        Serial.println(" esco da orologio");
        oraTime = false;
        a = 0;
        ctrlMessaggio = 1;
        newMessageAvailable = true;
        Serial.print("str in ora    "); Serial.println(strDaOra);
        messaggio();
      }
    }

  }
}
void messaggio() {
  Serial.print("ora time    "); Serial.println(oraTime);
  //TBMessage msg;
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
      ////////////////////////////////
      return;
    }


    TBMessage msg;   //// lo sposto allinizio TBMessage msg;
    if (myBot.getNewMessage(msg)) {

      if (msg.text.equalsIgnoreCase("ora")) {
        myBot.sendMessage(msg.sender.id, "Attivo orologio, digita qualcosa per uscire e mostrare il messaggio!");
        oraTime = true; ora();
      }

      if (msg.text.equalsIgnoreCase("u2753 Stato")) {
        Serial.print(" ora time"); Serial.println(oraTime);
        if (oraTime == 0) {
          myBot.sendMessage(msg.sender.id, "Sta passando il msg: " + String(newMessage));
          return;
        }
        if (oraTime == 1) {
          myBot.sendMessage(msg.sender.id, "Sono in orologio");
          return;
        }
      }

      if (msg.text.equalsIgnoreCase("/start")) {
        myBot.sendMessage(msg.sender.id, "Ciao, vuoi mandare in scrolling un messaggio?! Scrivilo semplicemente e invialo. "
                          " Per mostrare la tastiera digita 'tastiera'. "
                          " Il tasto 'Stato' ti informa cosa c'è al momento sul display. "
                          " Il tasto in basso fa passare il messaggio predefinito. "
                          " - by Davidevice - ");
        return;
      }


      if (msg.text.equalsIgnoreCase("aiuto")) {
        myBot.sendMessage(msg.sender.id, "Scrivi 'Ora' per mostrare l'orologio.Dall'orologio digita cio' che vuoi e verra' mostrato sul display.");
        return;
      }

      if (msg.text.equalsIgnoreCase("tastiera")) {
        // the user is asking to show the reply keyboard --> show it
        myBot.sendMessage(msg.sender.id, " ► TASTIERA ◄ ", myKbd);
        isKeyboardActive = true;
        return;
      }

      else if (msg.text.equalsIgnoreCase("ud83dudd51 Orologio")) {
        // the user is asking to show the reply keyboard --> show it
        myBot.sendMessage(msg.sender.id, "Attivo orologio, digita qualcosa per uscire e mostrare il messaggio!");
        Serial.println("Ahhhhhh");
        oraTime = true;
        // ctrlUScita = 1;   /// aggiunto per provare il panic esp8266
        ora();

      }

      else if (msg.text.equalsIgnoreCase("ud83dudee0 - Officina Vicenzi -")) {
        myBot.sendMessage(msg.sender.id, "sta scorrendo il messaggio predefinito!");
        myBot.sendMessage(msg.sender.id, "\U0001F6E0 'OFFICINA VICENZI'");
        ctrlUScita = 1;
        strDaOra = "-- OFFICINA VICENZI --";

      }

      else if (msg.text.equalsIgnoreCase("Orologio")) {
        // the user is asking to show the reply keyboard --> show it
        myBot.sendMessage(msg.sender.id, "Attivo orologio, digita qualcosa per uscire e mostrare il messaggio!");
        oraTime = true;
        ora();
      }

      //   else if (msg.messageType == CTBotMessageLocation) {
      //   // received a location message --> send a message with the location coordinates
      // myBot.sendMessage(msg.sender.id, "Longitude: " + (String)msg.location.longitude +
      // "\nLatitude: " + (String)msg.location.latitude);
      //  }



      if (ctrlUScita == 0) {
        Serial.print("ctr scita zero");
        String str = msg.text; Serial.print("MSG:TEXT"); Serial.println(str);
        int str_len = str.length() + 1;
        char char_array[str_len];

        str.toCharArray(char_array, str_len);

        strcpy (newMessage, char_array);
        Serial.println("sono alla fine del messaggio() " );
        myBot.sendMessage(msg.sender.id, "sta scorrendo il tuo messaggio: " + String(char_array) ); ////////////////////////////////
        newMessageAvailable = true;       // aggiunto l else

      }

      if (ctrlUScita == 1) {
        Serial.print("ctr scita uno"); Serial.println(strDaOra);
        Serial.print("MSG:TEXT due"); Serial.println(strDaOra);
        int str_len = strDaOra.length() + 1;
        char char_array[str_len];

        strDaOra.toCharArray(char_array, str_len);

        strcpy (newMessage, char_array);
        Serial.println("sono alla fine del messaggio() " );
        myBot.sendMessage(msg.sender.id, "sta scorrendo il tuo messaggio: "   + String(char_array));
        newMessageAvailable = true;       // aggiunto l else

      }

    }

    ctrlUScita = 0;
  }

  if (oraTime == true) {

    Serial.println("vado in ora");
    delay(1000);
    ora();


  }

}
