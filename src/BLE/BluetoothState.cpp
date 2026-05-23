#include <Arduino.h>
#include "BluetoothState.hpp"
#include "HAL/BLEController.hpp"

void BluetoothState::init(BLEController& HALObject, BLEEventQueue& eventQueue)
{
    Serial.println("Init BLE FSM");
    this->_HAL_Controller = &HALObject;
    this->_event_queue = &eventQueue;
}


void BluetoothState::handleEvent(BLE::Event event)
{
    
    switch (currentState)
    {
    case BluetoothState::State::ST_BT_IDLE:
        if(event == BLE::Event::BT_ADVERTISE_BTN_PRESSED){
            
            Serial.println("Starting Advertisement");
            this->_HAL_Controller->startAdvertising();
            digitalWrite(2, HIGH);
            this->currentState = State::ST_BT_ADVERTISE;
            
        }
        break;
    case State::ST_BT_ADVERTISE:
        if(event == BLE::Event::BT_CONNECTION_TIMEOUT){
            Serial.println("Connection Timeout, Going idle");
            this->currentState = State::ST_BT_IDLE;
            digitalWrite(2, LOW);

        }
        else if(event == BLE::Event::BT_CONNECTION_REQUEST){
            Serial.println("Connection Request Event!, Stopping advertisment");
            this->currentState = State::ST_BT_CONNECTED;
            
            this->_HAL_Controller->stopAdvertising();
            digitalWrite(2, LOW);
        }
        break;
    case State::ST_BT_CONNECTED:
        if(event == BLE::Event::BT_CONNECTION_LOST){
            
            this->currentState = State::ST_BT_CONN_RETRY;
            this->_HAL_Controller->startAdvertising();
            digitalWrite(2, HIGH);
        }
        break;
    case State::ST_BT_CONN_RETRY:
        if(event == BLE::Event::BT_CONNECTION_REQUEST){
            this->currentState = State::ST_BT_CONNECTED;
            this->_HAL_Controller->stopAdvertising();
            digitalWrite(2, LOW);
        }
        break;

    default:
        break;
    }
}

void BluetoothState::consumeAndHandleIncomingEvent()
{
    const BLE::Event incomingEvent = this->_event_queue->consumeEvent();
    handleEvent(incomingEvent);

}
