#pragma once

#include "BLE/HAL/BLEController.hpp"
#include "BLE/EventQueue/BLEEventQueue.hpp"
#include "BLE/BLEEvents.hpp"  


class BLEController;
class BLEEventQueue;

class BluetoothState {
    public:
        
        enum class State{
            ST_BT_ADVERTISE,
            ST_BT_IDLE, 
            ST_BT_CONNECTED,
            ST_BT_CONN_RETRY,
            ST_BT_DISCONNECTED, 
            ST_BT_TRANSMIT, 
            ST_BT_RECEIVE
        };
        
        void init(BLEController& HALObject, BLEEventQueue& eventQueue);
        

        void handleEvent(BLE::Event event);

        ///This Function should look into the Event queue and consume an event (or more) that updates the current state of the FSM
        
        void subscribeToEventQueue(); /// Switch to use of this function instead of having the init function do 2 thingss??
        void consumeAndHandleIncomingEvent(); 


        private:
        BLEController* _HAL_Controller;
        BLEEventQueue* _event_queue;
        State currentState = State::ST_BT_IDLE;

};