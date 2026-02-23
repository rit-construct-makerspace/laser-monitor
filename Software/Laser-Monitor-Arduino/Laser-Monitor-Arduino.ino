// Laser Monitor 
// This circuit monitors air pressure and fume extractor status for a laser cutter.

#define VERSION "V1.0.0-A"
#define HWVer "V1.0.0"

#include <avr/wdt.h>
#include <Wire.h>
#include <EEPROM.h>
#include "HX711.h"
#include <SparkFun_Alphanumeric_Display.h>

// Objects
HX711 air;
HT16K33 display;

// Constants (Pins and Memory Offsets)
const byte PIN_AIR_DATA      = PIN_PB1;
const byte PIN_AIR_CLOCK     = PIN_PB2;
const byte PIN_YELLOW        = PIN_PC0;
const byte PIN_GREEN         = PIN_PC1;
const byte PIN_BUZZER        = PIN_PC2;
const byte PIN_RED           = PIN_PE3;
const byte PIN_START_READ    = PIN_PD4;
const byte PIN_ERROR_READ    = PIN_PD2;
const byte PIN_LP_READ       = PIN_PC3;
const byte PIN_ACS_INTERRUPT = PIN_PD7;

const int ADDR_AIR_THRESHOLD = 0;
const int ADDR_START_DELAY   = 10;
const int ADDR_BRIGHTNESS    = 20;
const int ADDR_WARNING       = 30;

// Variables
byte brightness           = 0;
long airThreshold         = 0;
long startDelay           = 0;
unsigned long flashTimer  = 0;
bool flashState           = false;
bool filterWarning        = false;

// Function Prototypes
void runMonitor();
void printCurrentState();
void enterFaultState();
void setAndon(bool red, bool yellow, bool green, bool buzzer);
void setInterrupt(bool active);
bool isAirPressureOk();

void setup() {
  Serial.begin(115200);

  // Initialize Sensors and Displays
  air.begin(PIN_AIR_DATA, PIN_AIR_CLOCK);
  Wire.begin();
  display.begin();

  // Retrieve Settings from EEPROM
  EEPROM.get(ADDR_AIR_THRESHOLD, airThreshold);
  EEPROM.get(ADDR_START_DELAY, startDelay);
  EEPROM.get(ADDR_BRIGHTNESS, brightness);
  EEPROM.get(ADDR_WARNING, filterWarning);

  display.setBrightness(brightness);

  // Set Pin Modes
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_START_READ, INPUT);
  pinMode(PIN_ERROR_READ, INPUT);
  pinMode(PIN_LP_READ, INPUT);
}

void loop() {

  // Idle State display/lights
  if (digitalRead(PIN_START_READ)) {
    runMonitor();
  }
  
  display.print("STBY");
  setAndon(false, true, false, false);

  // Serial Command Handling
  if (Serial.available() >= 1) {
    String incoming = Serial.readString();
    incoming.trim();
    char command = incoming.charAt(0);

    if (command == 'c') {
      printCurrentState();
    } 
    else if (command == 'b') {
      incoming.remove(0, 2);
      int val = incoming.toInt();
      if (val <= 15) {
        brightness = val;
        EEPROM.put(ADDR_BRIGHTNESS, brightness);
        display.setBrightness(brightness);
      } else {
        Serial.println(F("ERROR: Brightness must be 0-15"));
      }
    } 
    else if (command == 'd') {
      incoming.remove(0, 2);
      startDelay = incoming.toInt();
      EEPROM.put(ADDR_START_DELAY, startDelay);
      Serial.print(F("Delay set to: "));
      Serial.print(startDelay);
      Serial.print(F(" milliseconds. ("));
      float airSeconds = startDelay / 1000.0;
      Serial.print(airSeconds);
      Serial.println(F(" seconds)."));
    } 
    else if (command == 'a') {
      incoming.remove(0, 2);
      airThreshold = incoming.toInt();
      EEPROM.put(ADDR_AIR_THRESHOLD, airThreshold);
      Serial.print(F("Air Threshold Set to: "));
      Serial.println(airThreshold);
    } 
    else if (command == 'v' || command == 'i'){
      Serial.println(F("Laser Monitor"));
      Serial.println(F("Developed by RIT SHED Makerspace"));
      Serial.println(F("make.rit.edu"));
      Serial.println(F("Licensed: CERN-OHL-S 2.0"));
      Serial.println(F("Source: https://github.com/rit-construct-makerspace/laser-monitor"));
      Serial.print(F("Hardware Version: ")); Serial.println(HWVer);
      Serial.print(F("Software Version: ")); Serial.println(VERSION);
    }
    else if (command == 'w'){
      incoming.remove(0, 2);
      filterWarning = incoming.toInt();
      EEPROM.put(ADDR_WARNING, filterWarning);
      if(filterWarning){
        Serial.println(F("Filter full warning enabled."));
      } else{
        Serial.println("Filter full warning disabled.");
      }
    }
    else {
      Serial.println(F("Commands: c (status), d [ms] (delay), a [val] (threshold), b [0-15] (bright), w [0/1] (filter warning), v (version), i (info)"));
    }
  }

  // Safety Reset if millis overflow is approaching (approx 34 days)
  if (millis() >= 3000000000) { 
    wdt_enable(WDTO_2S);
    while(1); 
  }

  delay(100);
}

void runMonitor() {
  unsigned long startTime = millis();
  bool blinking = false;
  bool FirstSerial = 0;
  
  while (digitalRead(PIN_START_READ)) {
    // 1. Determine State
    bool isStarting = (millis() - startTime < (unsigned long)startDelay);

    // 2. Serial Passthrough
    if (Serial.available()) {
      while(Serial.available()) Serial.read();
      if(!FirstSerial){
        FirstSerial = 1;
        Serial.println(F("Warning: while the laser is operating, all USB commands ignored except current state."));
      }
      printCurrentState();
    }

    // 3. Logic Branching
    if (isStarting) {
      display.print("WAIT");
      blinking = true;
    } 
    else {
      // Monitor Critical Faults
      if (!digitalRead(PIN_ERROR_READ)) {
        display.print("FUME");
        Serial.println(F("FAULT: Fume Extractor Offline!"));
        enterFaultState();
        break; 
      }

      if (!isAirPressureOk()) {
        display.print("AIR ");
        Serial.println(F("FAULT: Air Pressure Below Limit!"));
        enterFaultState();
        break;
      }

      // Check Non-Critical Warnings
      if (!digitalRead(PIN_LP_READ) && filterWarning) {
        display.print("FULL");
        blinking = true;
        Serial.println(F("WARNING: Low Pressure (Check Filter)"));
      } else {
        display.print("GOOD");
        blinking = false;
        setAndon(false, false, true, false);
      }
    }

    // 4. Handle Blinking Logic
    if (blinking) {
      if (millis() >= flashTimer) {
        flashTimer = millis() + 500;
        flashState = !flashState;
        setAndon(false, flashState, false, false);
      }
    }
  }
}

void printCurrentState() {
  Serial.println(F("\n--- System State ---"));
  Serial.print(F("Air Pressure: ")); Serial.println(air.read_average(10));
  Serial.print(F("Threshold:    ")); Serial.println(airThreshold);
  Serial.print(F("Laser Status: ")); Serial.println(digitalRead(PIN_START_READ) ? "RUNNING" : "IDLE");
  Serial.print(F("Fume Error:   ")); Serial.println(digitalRead(PIN_ERROR_READ) ? "OK" : "ERROR");
}

void enterFaultState() {
  printCurrentState();
  while (digitalRead(PIN_START_READ)) {
    setInterrupt(true);
    setAndon(true, false, false, true); // Red + Buzzer
  }
  setInterrupt(false);
}

void setAndon(bool red, bool yellow, bool green, bool buzzer) {
  digitalWrite(PIN_RED, red);
  digitalWrite(PIN_YELLOW, yellow);
  digitalWrite(PIN_GREEN, green);
  digitalWrite(PIN_BUZZER, buzzer);
}

void setInterrupt(bool active) {
  digitalWrite(PIN_ACS_INTERRUPT, LOW);
  if (active) {
    pinMode(PIN_ACS_INTERRUPT, OUTPUT);
  } else {
    pinMode(PIN_ACS_INTERRUPT, INPUT);
  }
}

bool isAirPressureOk() {
  return (air.read_average(10) > airThreshold);
}