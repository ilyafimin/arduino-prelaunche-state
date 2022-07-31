#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "Parser.h"

/* General Settings And Flags */
int comand[10];
int sensorN;
int readAllowed;

/* Temperature Sensor Settings */
#define ONE_WIRE_BUS 4
#define TEMPERATURE_PRECISION 12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorT(&oneWire);
DeviceAddress Thermometer;
float CURRENT_TEMP = 0.0;
double MAX_TEMP = 100.0;
double MIN_TEMP = 0.0;

/* GSR Sensor Settings */
const int GSR = A0;
int SENSORVALUE = 0;
int GSR_AVERAGE = 0;
      
/* Pulse Sensor Settings */
const byte RATE_SIZE = 4;
byte RATES[RATE_SIZE];  
byte RATESPOT = 0;
long LASTBEAT = 0;
float BEATSPERMINUTE;
int BEATAVG;      
MAX30105 PARTICLE_SENSOR;
/* --------------------- */

void sendPacket(int id);
void getTemperature();
void getPulse();
void getEAK();

void setup() {
  Serial.begin(115200);
  /* Temperature Sensor Initialization */
  sensorT.begin();
  sensorT.getAddress(Thermometer, 0);
  sensorT.setResolution(Thermometer, TEMPERATURE_PRECISION);
  /* Pulse Sensor Initialization */
  if (!PARTICLE_SENSOR.begin()) {
    Serial.println("404,2;");
    while (1);
  }
  PARTICLE_SENSOR.setup();
  PARTICLE_SENSOR.setPulseAmplitudeRed(0x0A);
  PARTICLE_SENSOR.setPulseAmplitudeGreen(0);  
}

void loop() {
  
  if (Serial.available()) {
     
     /* Getting paramets */
     char str[30];
     int amount = Serial.readBytesUntil(';', str, 30);
     str[amount] = 0;
     Parser data(str, ',');
     
     int am = data.parseInts(comand);
     /* ---------------- */
     switch(comand[0]){ 
      case 0: readAllowed = 0; break;
      case 1: {
        readAllowed = 1;
        sensorN = comand[1];     
        break;
      };
      default: Serial.println(403); break;
     };   
  } else {
    if (readAllowed) {
      switch(sensorN) {
        case 1: {
          getTemperature();
          sendPacket(1);
          break;
        };
        case 2: {
          getPulse();
          sendPacket(2);
          break;
        };
        case 3: {
          getEAK();
          sendPacket(3);
          break;
        };
      };
    };
  };
};

void sendPacket(int id) {
  char packet[40] = {0};
  switch(id) {
    case 1: {
      sprintf(packet, "%d,%d;", id, (int)(CURRENT_TEMP *10));
      break;
    };
    case 2: {
;     sprintf(packet, "%d,%d,%d;", id, (int)BEATSPERMINUTE, BEATAVG);
      break;
    };
    case 3: {
;     sprintf(packet, "%d,%d;", id, GSR_AVERAGE);
      break;
    };
    default: {
      sprintf(packet, "%d,%d,%d;", 1, (int)MIN_TEMP, (int)MAX_TEMP);
      sprintf(packet, "%d,%d,%d;", 2, (int)BEATSPERMINUTE, BEATAVG);
      sprintf(packet, "%d;", 3, GSR_AVERAGE);
    };
  };
  Serial.println(packet);
};

void getTemperature(){
  sensorT.requestTemperatures();
  CURRENT_TEMP = sensorT.getTempC(Thermometer);
};

void getPulse(){
  long irValue = PARTICLE_SENSOR.getIR();
  if (checkForBeat(irValue) == true) {
    long DELTA = millis() - LASTBEAT;
    LASTBEAT = millis();
    BEATSPERMINUTE = 60 / (DELTA / 1000.0);
    if (BEATSPERMINUTE < 255 && BEATSPERMINUTE > 20) {
      RATES[RATESPOT++] = (byte)BEATSPERMINUTE;
      RATESPOT %= RATE_SIZE;
      BEATAVG = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++) {
        BEATAVG += RATES[x];
      };
      BEATAVG /= RATE_SIZE;
    };
//    start_time = millis() /1000;
  };
};

void getEAK() {
  long sum = 0;
  for(int i=0; i<10; i++) {
    SENSORVALUE = analogRead(GSR);
    sum += SENSORVALUE;
    delay(5);
  };
  GSR_AVERAGE = sum / 10;
};
