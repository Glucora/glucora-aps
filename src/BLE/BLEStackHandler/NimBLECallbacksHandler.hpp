#pragma once

#include <NimBLEDevice.h>
#include <NimBLEServer.h>

class BLEEventQueue;

class BLEServerCallbacksHandler : public NimBLEServerCallbacks{
    public: 
        BLEServerCallbacksHandler();
        BLEServerCallbacksHandler(BLEEventQueue& _event_queue);
        void onConnect(NimBLEServer *pserver, NimBLEConnInfo &connInfo);
        void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo& connInfo, int reason);
        void startAdvertising(NimBLEAdvertising *pAdv);
    private:
        BLEEventQueue* _event_queue;
};