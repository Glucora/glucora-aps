#pragma once

#include <NimBLEDevice.h>
#include <NimBLEAdvertising.h>
#include "BLE/BLEStackHandler/NimBLECallbacksHandler.hpp"
#include "BLE/BLEEvents.hpp"

class BLEEventQueue; 

class BLEController{
    public:
    using GlucoseSampleHandler = void (*)(float);

        void startAdvertising();
        void stopAdvertising();
        void createSerivceAndCharacteristic(const char *serviceId, const char* characteristicId);
        
        template<typename T>
        void upsertAndNotifyCharacteristic(const char *serviceId, const char *charId, T data);


        void init(BLEEventQueue& eventQueue);
        void createService(const char* serviceId);
        void startService(const char* serviceId);
    void initGlucoseClient(const char* peerAddress,
                 const char* serviceUuid,
                 const char* characteristicUuid,
                 GlucoseSampleHandler sampleHandler);
    void maintainGlucoseClientConnection();
    bool isGlucoseClientConnected() const;

    void handleGlucoseClientDisconnect(int reason);

    private:
  bool connectGlucoseClient();
  bool subscribeToGlucoseCharacteristic();

  static void glucoseNotifyCallback(NimBLERemoteCharacteristic* pCharacteristic,
                    uint8_t*                    pData,
                    size_t                      length,
                    bool                        isNotify);


    NimBLEAdvertising *pAdvertising = nullptr;
    NimBLEServer* pServer = nullptr;
  NimBLEClient* pClient = nullptr;
  NimBLERemoteCharacteristic* pRemoteCharacteristic = nullptr;
    BLEEventQueue* _event_queue;
  const char* _peerAddress = nullptr;
  const char* _serviceUuid = nullptr;
  const char* _characteristicUuid = nullptr;
  GlucoseSampleHandler _sampleHandler = nullptr;
  unsigned long _lastReconnectAttempt = 0;
  unsigned long _reconnectDelayMs = 2000;
  bool _glucoseClientConnected = false;
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