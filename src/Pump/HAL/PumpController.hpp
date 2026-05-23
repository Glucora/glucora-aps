#pragma once 
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <vector>

#define PUMP_PIN 26  
#define PUMP_FLOW_RATE 0.00801410f   //microliters per ms
#define IOB_DECAY_HALF_LIFE_MS (4 * 60 * 60 * 1000)  // 4 hours in milliseconds

// Callback typedef for delivery events: called with delivered units and new activeIOB
using DeliveryCallback = void (*)(float deliveredUnits, float activeIOB);

// Struct to track individual deliveries for IOB decay calculation
struct InsulinDelivery {
  float units;
  unsigned long timestampMs;
};

class PumpHAL{

    private:
    
    bool currentState;
    volatile static bool desiredState;
    volatile static bool isTimerFired;
    TimerHandle_t dosageTimer = NULL;
    
    // Insulin tracking and persistence
    float totalDeliveredU = 0.0f;  // cumulative total delivered, persisted
    float lastDosageUnits = 0.0f;  // units delivered in current/last pump cycle
    std::vector<InsulinDelivery> deliveryHistory;  // recent deliveries for IOB decay
    unsigned long lastDeliveryTimestampMs = 0;
    DeliveryCallback onDeliveryCallback = nullptr;
    
    static void onTimerCallback(TimerHandle_t xTimer);
    void startPump();
    void stopPump();
    void setupDosageTimer(int timeMS);
    void recordDelivery(float units);  // internal method called after pump stops
    void loadPersistence();
    void savePersistence();
    

  public:

    PumpHAL();
    void update();
    void deliverUnits(uint8_t basalUnits);
    
    // Get the current active insulin on board (decayed over time)
    float getActiveIOB() const;
    
    // Get total insulin delivered (cumulative)
    float getTotalDelivered() const;
    
    // Set callback to be invoked when delivery completes
    void setDeliveryCallback(DeliveryCallback callback);

};