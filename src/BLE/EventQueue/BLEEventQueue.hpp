#pragma once
#include <queue>

#include "BLE/BLEEvents.hpp" 

#define MAX_QUEUE_EVENTS 10




class BLEEventQueue{
    private:
    std::queue<BLE::Event> eventQueue;
    

    public:
    void init();
    bool createEvent(BLE::Event event);
    BLE::Event consumeEvent();

};