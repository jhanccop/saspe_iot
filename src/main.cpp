/*
Siren SASpe

Esp32 - sim800 - RTC - mqtt
*/

/* ====================== LCD CONFIG ======================== */
#include <Arduino.h>

#define _Digole_Serial_I2C_
#include <DigoleSerial.h>

#include <Wire.h>
DigoleSerialDisp mydisp(&Wire,'\x27');

const unsigned char fonts[] = {6, 10, 18, 51, 120, 123};
const char *fontdir[] = {"0\xb0", "90\xb0", "180\xb0", "270\xb0"};

/* ====================== RTC CONFIG ======================== */
#include "RTClib.h"
RTC_DS3231 rtc;

/* ====================== GRPS CONFIG ======================= */

/* settings grps apn */
const char apn[]  = "claro.pe";
const char gprsUser[] = "clar@datos";
const char gprsPass[] = "";

/* pin sim */
const char simPIN[]   = "";
#define MODEM_TX        16 // 16
#define MODEM_RX        17 // 17
#define reset_modem           5

/* Set serial for AT commands (to SIM800 module) */
#define SerialAT Serial1 // Serial1

/* Configure TinyGSM library */
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800

#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

#define TINY_GSM_USE_GPRS true

#include <TinyGsmClient.h>

#define DUMP_AT_COMMANDS
TinyGsm modem(SerialAT);

TinyGsmClient client(modem);

/* ====================== MQTT CONFIG ======================== */

#include <PubSubClient.h>

PubSubClient mqtt(client);

/* settings MQTT */
const char *broker = "broker.mqttdashboard.com";
const int mqtt_port = 1883;
const char *mqtt_user = "device01";
const char *mqtt_pass = "sasppe*&^";

const String serial_number = "00001";

const char* topicData_pub = "saspe/sirenData";
const char* topicInit_pub = "saspe/init";
const char* topicSirenStatus_subs = "saspe/siren/#";

long lastMsg = 0;
char msg[50];
char msg_c[50];

unsigned long now = 0;
//unsigned long previousMillis = 0; // init millis wait to mqtt callback
//const long interval = 5000; // secont wait to mqtt callback


/* ========================== times ========================== */
unsigned long previousMillis = 0; // init millis wait to mqtt callback
const long interval = 5000; // secont wait to mqtt callback

String dateTime = "";   // variable to datetime

long refresh = 30000;
unsigned long time_now = 0;

unsigned long timeStart = 0;
unsigned long timeEnd = 0;
unsigned long deltaTime = 0;

/* ====================== SEPARADOR ======================== */
#include <Separador.h>
Separador s;

/* ************************************************************************************************************** */
/* ************************************************* FUNCTIONS ************************************************** */
/* ************************************************************************************************************** */

boolean setup_grps();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnect();


/* ******************* PRINT MESSAGE  ********************** */
void print_text(String msg, int x, int y) {
    mydisp.setFont(fonts[0]);
    mydisp.setPrintPos(x, y, _TEXT_);
    mydisp.print(msg);
}


/* ******************* PRINT DATETIME  ********************** */
void print_datetime(boolean show) {
    mydisp.setFont(fonts[0]);
    
    DateTime now = rtc.now();
    String year = String(now.year());
    //year.remove(0,2);
    String datetime = year + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

    if(show){
        mydisp.setPrintPos(14, 0, _TEXT_);
        
    }
    else{
        mydisp.setPrintPos(6, 2, _TEXT_);
    }
    mydisp.print(datetime);
}


/* ******************* MQTT CALLBACK  ********************** */
void mqttCallback(char* topic, byte* payload, unsigned int length) {

    char buff[length];
    for (int i = 0; i < length; i++)
    {
        buff[i] = (char)payload[i];
    }
    String message = String(buff);


    if(String(topic) == "saspe/siren/setRTC"){
        timeEnd = millis();

        deltaTime = (timeEnd - timeStart)*0.001;

        int setting[6];

        for (int j = 0; j < 6; j++)
        {
            setting[j] = s.separa(message, ',', j).toInt();
        }

        rtc.adjust(DateTime(setting[0], setting[1], setting[2], setting[3], setting[4], setting[5] + deltaTime*0.5));
        print_datetime(true);

    }else{
        print_text(message, 6, 4);
        print_datetime(false);
    }

}

/* ******************* RECONNECT  ********************** */
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    print_text("MQTT connection...       ", 0, 9);
    // Attempt to connect

    if (mqtt.connect(mqtt_user)) {
      Serial.println("connected");
      mqtt.subscribe(topicSirenStatus_subs);
      print_text("connected", 17, 9);
    } else {
      Serial.print("failed, rc="); 
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      print_text("failed   ", 17, 9);
      delay(5000);
    }
  }
}

/* ********************** RESET POSITION *********************** */
void resetpos1(void) //for demo use, reset display position and clean the demo line
{
    mydisp.setPrintPos(0, 0, _TEXT_);
    delay(3000); //delay 2 seconds
    mydisp.println("                "); //display space, use to clear the demo line
    mydisp.setPrintPos(0, 0, _TEXT_);
}

/* ********************** LCD PREPARE *********************** */
void lcd_prepare(void) {
    delay(100);
    
    mydisp.backLightOn();
    mydisp.setColor(1);
    mydisp.clearScreen();
    mydisp.setFont(fonts[0]);
    uint8_t i = 2;
    mydisp.setRotation(i);
    mydisp.drawStr(0, 0, "saspe v1");
    mydisp.drawLine(0, 6, 127, 6);

    mydisp.drawStr(0, 2, "Tarr>");
    mydisp.drawStr(0, 3, "Topi>");
    mydisp.drawStr(0, 4, "Mess>");
    mydisp.drawStr(0, 5, "Late>");
    mydisp.drawStr(0, 6, "Sire>");

}


/* ************************* SETUP GRPS ************************** */
boolean setup_grps() {

    mydisp.clearScreen();
    mydisp.setFont(fonts[0]);
    mydisp.setRotation(2);
    mydisp.setPrintPos(0, 0, _TEXT_);
    mydisp.print(">> Welcome to siren saspe");

    // Set GSM module baud rate and UART pins
    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

    mydisp.setPrintPos(0, 1, _TEXT_);
    mydisp.print(">> Init modem");
    //modem.restart();

    String modemInfo = modem.getModemInfo();
    mydisp.setPrintPos(0, 2, _TEXT_);
    mydisp.print(modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
        modem.simUnlock(simPIN);
        mydisp.print(" simUnlock");
    }

    if (modemInfo == "") {
        mydisp.print(" fail");
        return false;
    }
    mydisp.setPrintPos(0, 3, _TEXT_);
    mydisp.print(">> Connecting...");
    if (!modem.waitForNetwork(240000L)) {
        mydisp.print(" fail");
        return false;
    }else{mydisp.print(" ok");}

    mydisp.setPrintPos(0, 4, _TEXT_);
    mydisp.print(">> Connection...");

    if (modem.isNetworkConnected()) {
        mydisp.print(" to claro");
    }
    else {
        mydisp.print(" error");
        return false;
    }

    mydisp.setPrintPos(0, 5, _TEXT_);
    mydisp.print(">> GRPS...");

    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        mydisp.print(" fail");
        delay(2000);
        return false;
    } else {
        mydisp.print(" ok"); // sim  grps ok
        delay(2000);
        return true;
    }
    //delay(12000);
    mydisp.clearScreen();

}

/* ******************* SETUP  ********************** */
void setup(){
    // reset modem
    pinMode(reset_modem,OUTPUT);
    digitalWrite(reset_modem, LOW);
    delay(500);
    digitalWrite(reset_modem, HIGH);

    mydisp.begin();
    delay(3000);
    //lcd_prepare();
    setup_grps();
    lcd_prepare();
    if (! rtc.begin()) {
        abort();
    }
    print_datetime(true);

    Serial.begin(9600);

    mqtt.setServer(broker, mqtt_port);
    mqtt.setCallback(mqttCallback);

/* **************** INIT AND SET RTC ********************* */
    if (mqtt.connect(mqtt_user)) {
        mqtt.publish(topicInit_pub,"start");
        timeStart = millis();
        mqtt.subscribe(topicSirenStatus_subs);
        print_text(topicSirenStatus_subs, 6, 3);
    }

}

void loop()
{
    if (!mqtt.connected()) {
        reconnect();
    }
    
    mqtt.loop();

    if (millis() > now + 5000)
    {

        print_datetime(true);
        char data[150];
        String payload = String(analogRead(34));
        payload.toCharArray(data, (payload.length() + 1));

        mqtt.publish(topicData_pub, data);

        now = millis();
    }
}
