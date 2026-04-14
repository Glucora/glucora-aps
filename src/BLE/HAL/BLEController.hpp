#pragma once

#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>
#include "BLE/BLEStackHandler/NimBLECallbacksHandler.hpp"
#include "BLE/BLEEvents.hpp"

class BLEEventQueue; 

class BLEController{
    public:
        void startAdvertising();
        void stopAdvertising();
        void createSerivceAndCharacteristic(const char *serviceId, const char* characteristicId);
        
        template<typename T>
        void upsertAndNotifyCharacteristic(const char *serviceId, const char *charId, T data);


        void init(BLEEventQueue& eventQueue);
        void createService(const char* serviceId);
        void startService(const char* serviceId);

    private:


    NimBLEAdvertising *pAdvertising = nullptr;
    NimBLEServer* pServer = nullptr;
    BLEEventQueue* _event_queue;
};

template<typename T>
void BLEController::upsertAndNotifyCharacteristic(const char *serviceId, const char *charId, T data)
{
  NimBLEService *pService = this->pServer->getServiceByUUID(serviceId);
  if(!pService){
    return; ///if control flow reaches this the BLE Service wasn't Initialized..
  }
  NimBLECharacteristic *pChar = pService->getCharacteristic(charId);
  if(!pChar){
    return;
  }
  pChar->setValue(data);
  pChar->notify();
}