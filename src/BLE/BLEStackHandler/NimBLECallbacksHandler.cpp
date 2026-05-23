#include "NimBLECallbacksHandler.hpp"
#include "BLE/EventQueue/BLEEventQueue.hpp"



BLEServerCallbacksHandler::BLEServerCallbacksHandler(){}
BLEServerCallbacksHandler::BLEServerCallbacksHandler(BLEEventQueue& _event_queue){
    this->_event_queue = &_event_queue;
}

void BLEServerCallbacksHandler::onConnect(NimBLEServer *pserver, NimBLEConnInfo &connInfo)
{

    Serial.println("Clinet Connected!");
    Serial.println(connInfo.getAddress().toString().c_str());
    this->_event_queue->createEvent(BLE::Event::BT_CONNECTION_REQUEST);
    
}

void BLEServerCallbacksHandler::onDisconnect(NimBLEServer *pServer, NimBLEConnInfo& connInfo, int reason){
    Serial.println("Clinet Disconnected!");
    Serial.printf("Reason: %d\n", reason);
    pServer->startAdvertising();
    this->_event_queue->createEvent(BLE::Event::BT_CONNECTION_LOST);
    
}

void onAuthenticationComplete(NimBLEConnInfo& connInfo){
    if(!connInfo.isBonded()){
        NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
    }
}

void BLEServerCallbacksHandler::startAdvertising(NimBLEAdvertising *pAdv)
{
    Serial.println("Starting Advertising!");
}
