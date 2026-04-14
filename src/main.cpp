#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "BLE/BluetoothState.hpp"

#include "BLE/BLEStackHandler/NimBLECallbacksHandler.hpp"
#include "BLE/HAL/BLEController.hpp"
#include "BLE/EventQueue/BLEEventQueue.hpp"
#include "Predictors/MAL/PredictorController.hpp"


BLEController BLE_HAL;
BluetoothState BTFSM;
BLEEventQueue BLEQeue;
PredictorController predictionController;

static unsigned long lastInference = 0;
static unsigned int inferenceDelay = 5000;

int random_index(int length) {
    if (length <= 0) return -1;
    return rand() % length;
}

bool buttonPressed(uint8_t btnPin) {
  static unsigned long lastTime = 0;
  static bool lastState = HIGH;

  bool state = digitalRead(btnPin);
  if (state != lastState && millis() - lastTime > 50) {
    lastState = state;
    lastTime = millis();
    if (state == LOW) return true;  // Active-low button press
  }
  return false;
}

const float glucose_readings[24] = {70, 83, 85, 86, 90, 90, 91, 92, 95, 96, 98, 104, 105, 111, 116, 121, 124, 126, 127, 130, 135, 136, 138, 121}; ///This is Dummy Data


void setup(){   

  Serial.begin(115200);
  srand(time(NULL)); 
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  
  BLE_HAL.init(BLEQeue);
  ///change this to an ENUM/lookup table to avoid raw UUIDS
  BLE_HAL.createService("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12");
  
  

  BLE_HAL.createSerivceAndCharacteristic("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12", "AB12A1A2-AB12-AB12-AB12-AB12AB12AB12");
  
  BLE_HAL.createSerivceAndCharacteristic("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12", "AB12A2A4-AB12-AB12-AB12-AB12AB12AB12");  
  
  BLE_HAL.createSerivceAndCharacteristic("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12", "AB12A4A8-AB12-AB12-AB12-AB12AB12AB12");
  
  BLE_HAL.startService("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12");
  

  

  BLE_HAL.createService("0000180F-0000-1000-8000-00805f9b34fb");
  
  BLE_HAL.createSerivceAndCharacteristic("0000180F-0000-1000-8000-00805f9b34fb", "00002A19-0000-1000-8000-00805f9b34fb");
  
  BLE_HAL.startService("0000180F-0000-1000-8000-00805f9b34fb");
  


  BLE_HAL.upsertAndNotifyCharacteristic("0000180F-0000-1000-8000-00805f9b34fb", "00002A19-0000-1000-8000-00805f9b34fb", 67);
  BLE_HAL.upsertAndNotifyCharacteristic("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12", "AB12A2A4-AB12-AB12-AB12-AB12AB12AB12", 3.2f);

  

  BTFSM.init(BLE_HAL, BLEQeue);

  pinMode(25, INPUT_PULLUP);
  

  predictionController.init();

  
  digitalWrite(2, LOW);
}

void loop(){


  BTFSM.consumeAndHandleIncomingEvent();

  if(buttonPressed(25)){
    
    BTFSM.handleEvent(BLE::Event::BT_ADVERTISE_BTN_PRESSED);
  }

  if(millis() - lastInference >= inferenceDelay){
    
    lastInference = millis();
    const float reading = glucose_readings[random_index(24)];
    predictionController.feed(reading);
    BLE_HAL.upsertAndNotifyCharacteristic("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12", "AB12A4A8-AB12-AB12-AB12-AB12AB12AB12", reading);
    if(predictionController.isReady()){
      Serial.println("Prediction Ready, streaming data!");
      const int samplePrediction = predictionController.predict();
      BLE_HAL.upsertAndNotifyCharacteristic("AB12ABAB-AB12-AB12-AB12-AB12AB12AB12", "AB12A1A2-AB12-AB12-AB12-AB12AB12AB12", samplePrediction);
      
      
    }
  }
  
  
}