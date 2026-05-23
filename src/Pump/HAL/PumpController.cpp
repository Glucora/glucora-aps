#include "PumpController.hpp"
#include <Preferences.h>

volatile bool PumpHAL::desiredState = false;
volatile bool PumpHAL::isTimerFired = false;

// Static callback wrapper for FreeRTOS timer
void PumpHAL::onTimerCallback(TimerHandle_t xTimer)
{
    PumpHAL* pumpPtr = (PumpHAL*)pvTimerGetTimerID(xTimer);
    if(pumpPtr != NULL) {
        // Record the delivery before stopping the pump
        pumpPtr->recordDelivery(pumpPtr->lastDosageUnits);
        desiredState = false;
        isTimerFired = false;
    }
}

void PumpHAL::startPump()
{
    Serial.println("Starting pump via GPIO");
    digitalWrite(PUMP_PIN, HIGH);
    currentState = true;
 
}

void PumpHAL::stopPump()
{
    
    Serial.println("STOPPING pump via GPIO");
    digitalWrite(PUMP_PIN, LOW);
    currentState = false;

}

void PumpHAL::setupDosageTimer(int timeMS)
{
    // Clean up existing timer
    if(this->dosageTimer != NULL){
        xTimerStop(this->dosageTimer, portMAX_DELAY);
        xTimerDelete(this->dosageTimer, portMAX_DELAY);
        this->dosageTimer = NULL;
    }

    // Create FreeRTOS software timer (doesn't conflict with BLE hardware timers)
    this->dosageTimer = xTimerCreate(
        "PumpTimer",
        pdMS_TO_TICKS(timeMS),
        pdFALSE,  // one-shot timer
        (void*)this,  // timer ID
        PumpHAL::onTimerCallback
    );

    if(this->dosageTimer != NULL) {
        isTimerFired = true;
        xTimerStart(this->dosageTimer, portMAX_DELAY);
    } else {
        Serial.println("ERROR: Failed to create pump timer!");
    }
}

PumpHAL::PumpHAL()
{
    pinMode(PUMP_PIN, OUTPUT);
    currentState = false;
    loadPersistence();
}

void PumpHAL::update()
{
    if(!isTimerFired && this->dosageTimer != NULL){
        xTimerStop(this->dosageTimer, portMAX_DELAY);
        xTimerDelete(this->dosageTimer, portMAX_DELAY);
        this->dosageTimer = NULL;
    }

    if(currentState != desiredState){
        currentState ?  stopPump() : startPump() ;
    }
}

//! This assumes U-100 insulin and nothing else
/// @brief This Function converts basal units to microliters and dispatches a pumping order with the required time
/// @param basalUnits ammount of standard rapid U-100 units
void PumpHAL::deliverUnits(uint8_t basalUnits)
{
    if(isTimerFired) return; //pump already active and delivery

    const int microliterDose = basalUnits * 10; // 1ml of insulin is 100 units
    const int dosingTimeMS = microliterDose / PUMP_FLOW_RATE;

    Serial.printf("Dosing time: %d ms for %d units\n", dosingTimeMS, basalUnits);

    lastDosageUnits = (float)basalUnits;  // Store units for delivery tracking
    desiredState = true;
    this->setupDosageTimer(dosingTimeMS);
}

/// @brief Record a delivery and notify via callback with new IOB
/// @param units Amount of insulin delivered in units
void PumpHAL::recordDelivery(float units)
{
    if(units <= 0.0f) return;  // Ignore zero deliveries
    
    unsigned long nowMs = millis();
    
    // Add to delivery history for IOB decay calculation
    deliveryHistory.push_back({units, nowMs});
    
    // Update cumulative total
    totalDeliveredU += units;
    lastDeliveryTimestampMs = nowMs;
    
    // Save to persistent storage
    savePersistence();
    
    // Calculate updated active IOB
    float activeIOB = getActiveIOB();
    
    Serial.printf("Pump delivery recorded: %.2f units, total: %.2f units, active IOB: %.2f units\n", 
                  units, totalDeliveredU, activeIOB);
    
    // Invoke callback if set
    if(onDeliveryCallback != nullptr) {
        onDeliveryCallback(units, activeIOB);
    }
}

/// @brief Get the current active insulin on board with exponential decay
/// @return Current IOB in units, accounting for exponential decay
float PumpHAL::getActiveIOB() const
{
    unsigned long nowMs = millis();
    float activeIOB = 0.0f;
    
    // Calculate decay for each delivery in history
    // Using exponential decay: remaining = units * 0.5^(elapsed / half_life)
    for(const auto& delivery : deliveryHistory) {
        unsigned long elapsedMs = nowMs - delivery.timestampMs;
        // Decay constant for exponential decay: 0.5^(elapsed / half_life)
        float decayFactor = pow(0.5f, (float)elapsedMs / (float)IOB_DECAY_HALF_LIFE_MS);
        activeIOB += delivery.units * decayFactor;
    }
    
    return activeIOB;
}

/// @brief Get total insulin delivered (cumulative, not decayed)
/// @return Cumulative units delivered since initialization
float PumpHAL::getTotalDelivered() const
{
    return totalDeliveredU;
}

/// @brief Set a callback to be invoked when delivery completes
/// @param callback Function pointer to be called with (deliveredUnits, activeIOB)
void PumpHAL::setDeliveryCallback(DeliveryCallback callback)
{
    onDeliveryCallback = callback;
}

/// @brief Load persisted delivery total from NVS
void PumpHAL::loadPersistence()
{
    Preferences prefs;
    prefs.begin("pump", true);  // Read-only mode
    totalDeliveredU = prefs.getFloat("totalDeliveredU", 0.0f);
    prefs.end();
    
    Serial.printf("Loaded persisted total: %.2f units\n", totalDeliveredU);
}

/// @brief Save delivery total to NVS for persistence across reboots
void PumpHAL::savePersistence()
{
    Preferences prefs;
    prefs.begin("pump", false);  // Read-write mode
    prefs.putFloat("totalDeliveredU", totalDeliveredU);
    prefs.end();
}
