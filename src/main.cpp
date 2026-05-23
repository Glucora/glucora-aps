#include <Arduino.h>
#include <stdio.h>
#include <math.h>

#include "BLE/BluetoothState.hpp"

#include "BLE/BLEStackHandler/NimBLECallbacksHandler.hpp"
#include "BLE/HAL/BLEController.hpp"
#include "BLE/EventQueue/BLEEventQueue.hpp"
#include "Predictors/MAL/PredictorController.hpp"
#include "Pump/HAL/PumpController.hpp"

BLEController BLE_HAL;
BluetoothState BTFSM;
BLEEventQueue BLEQeue;
PredictorController predictionController;
PumpHAL pumpController;

static bool pendingPrediction = false;

static const char* GLUCOSE_PEER_ADDRESS = "28:56:2F:49:A2:5A";
static const char* GLUCOSE_SERVICE_UUID = "12345678-1234-1234-1234-123456789abc";
static const char* GLUCOSE_CHARACTERISTIC_UUID = "87654321-4321-4321-4321-cba987654321";

static const char* LOCAL_SERVICE_UUID = "AB12ABAB-AB12-AB12-AB12-AB12AB12AB12";
static const char* LOCAL_PREDICTION_UUID = "AB12A1A2-AB12-AB12-AB12-AB12AB12AB12";
static const char* LOCAL_READING_UUID = "AB12A2A4-AB12-AB12-AB12-AB12AB12AB12";
static const char* LOCAL_FEED_UUID = "AB12A4A8-AB12-AB12-AB12-AB12AB12AB12";
static const char* LOCAL_DELIVERED_UUID = "AB12A5B2-AB12-AB12-AB12-AB12AB12AB12";  // New: pump delivered units notification

static const float GLUCOSE_MIN = 47.0f;
static const float GLUCOSE_MAX = 400.0f;

float denormalizeGlucose(float normalized) {
  if (normalized < 0.0f) normalized = 0.0f;
  if (normalized > 1.0f) normalized = 1.0f;
  return (normalized * (GLUCOSE_MAX - GLUCOSE_MIN)) + GLUCOSE_MIN;
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

void onGlucoseSampleReceived(float normalizedReading) {
  if (!isfinite(normalizedReading)) {
    Serial.println("Ignoring invalid glucose reading");
    return;
  }

  const float reading = denormalizeGlucose(normalizedReading);
  Serial.printf("CGM normalized %.3f -> denormalized %.1f mg/dL\n", normalizedReading, reading);

  pendingPrediction = true;

  if (reading > 95) {
    Serial.println("High Level, Delivering correction!");
    pumpController.deliverUnits(2);
  }

  predictionController.feed(reading);
  BLE_HAL.upsertAndNotifyCharacteristic(LOCAL_SERVICE_UUID, LOCAL_FEED_UUID, reading);
}

/// @brief Callback invoked when pump delivers insulin
/// @param deliveredUnits Amount of insulin just delivered (units)
/// @param activeIOB Current active insulin on board after delivery (units)
void onPumpDelivered(float deliveredUnits, float activeIOB) {
  // Send notification: pump delivered n units
  BLE_HAL.upsertAndNotifyCharacteristic(LOCAL_SERVICE_UUID, LOCAL_DELIVERED_UUID, deliveredUnits);
  
  // Update reading characteristic with new active IOB
  BLE_HAL.upsertAndNotifyCharacteristic(LOCAL_SERVICE_UUID, LOCAL_READING_UUID, activeIOB);
  
  Serial.printf("BLE notifications sent: delivered=%.2f units, IOB=%.2f units\n", deliveredUnits, activeIOB);
}


void setup(){   

  Serial.begin(115200);
  pinMode(2, OUTPUT);
  digitalWrite(2, HIGH);
  

  
  BLE_HAL.init(BLEQeue);
    predictionController.init();


  ///change this to an ENUM/lookup table to avoid raw UUIDS
  BLE_HAL.createService(LOCAL_SERVICE_UUID);
  
  

  BLE_HAL.createSerivceAndCharacteristic(LOCAL_SERVICE_UUID, LOCAL_PREDICTION_UUID);
  
  BLE_HAL.createSerivceAndCharacteristic(LOCAL_SERVICE_UUID, LOCAL_READING_UUID);  
  
  BLE_HAL.createSerivceAndCharacteristic(LOCAL_SERVICE_UUID, LOCAL_FEED_UUID);
  
  BLE_HAL.createSerivceAndCharacteristic(LOCAL_SERVICE_UUID, LOCAL_DELIVERED_UUID);  // New: delivered units
  
  BLE_HAL.startService(LOCAL_SERVICE_UUID);
  

  

  BLE_HAL.createService("0000180F-0000-1000-8000-00805f9b34fb");
  
  BLE_HAL.createSerivceAndCharacteristic("0000180F-0000-1000-8000-00805f9b34fb", "00002A19-0000-1000-8000-00805f9b34fb");
  
  BLE_HAL.startService("0000180F-0000-1000-8000-00805f9b34fb");
  


  BLE_HAL.upsertAndNotifyCharacteristic("0000180F-0000-1000-8000-00805f9b34fb", "00002A19-0000-1000-8000-00805f9b34fb", 67);
  
  // Broadcast current insulin on board (dynamic, based on persisted deliveries and decay)
  float currentIOB = pumpController.getActiveIOB();
  BLE_HAL.upsertAndNotifyCharacteristic(LOCAL_SERVICE_UUID, LOCAL_READING_UUID, currentIOB);
  
  // Set up pump delivery callback to send BLE notifications
  pumpController.setDeliveryCallback(onPumpDelivered);

  BLE_HAL.initGlucoseClient(GLUCOSE_PEER_ADDRESS,
                            GLUCOSE_SERVICE_UUID,
                            GLUCOSE_CHARACTERISTIC_UUID,
                            onGlucoseSampleReceived);

  
  


  BTFSM.init(BLE_HAL, BLEQeue);
  BLE_HAL.startAdvertising();

  pinMode(33, INPUT_PULLUP);
  pinMode(25, INPUT_PULLUP);
  pinMode(26, OUTPUT);

  
  digitalWrite(2, LOW);
}

bool pumpState = false;
unsigned long loopStart;
void loop(){
  pumpController.update();

  if(digitalRead(33) == LOW){
    if(!pumpState){
      pumpState = true;
      loopStart = millis();
    }
    digitalWrite(2, HIGH);
    digitalWrite(26, HIGH);

  }
  else{
    if(pumpState){
      unsigned long loopEnd = millis();
      uint16_t elapsedTime = loopEnd - loopStart;
      Serial.printf("====Elapsed Time======\n%u\n===============\n", elapsedTime);
      digitalWrite(26, LOW);
      digitalWrite(2, LOW);
      pumpState = false;
    }



  }
  BTFSM.consumeAndHandleIncomingEvent();

  BLE_HAL.maintainGlucoseClientConnection();

  if(buttonPressed(25)){
    
    BTFSM.handleEvent(BLE::Event::BT_ADVERTISE_BTN_PRESSED);
  }

  if(pendingPrediction && predictionController.isReady()){
    digitalWrite(2, HIGH);

    pendingPrediction = false;
    Serial.println("Prediction Ready, streaming data!");
    const int samplePrediction = predictionController.predict();
    BLE_HAL.upsertAndNotifyCharacteristic(LOCAL_SERVICE_UUID, LOCAL_PREDICTION_UUID, samplePrediction);

    digitalWrite(2, LOW);
  }
  
  
}