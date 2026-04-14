#include "BLEEventQueue.hpp"

void BLEEventQueue::init()
{

}

bool BLEEventQueue::createEvent(BLE::Event event)
{
    if(this->eventQueue.size() > MAX_QUEUE_EVENTS){
        return false; ///faliure to create event
    }
    this->eventQueue.push(event);
    return true;

}

BLE::Event BLEEventQueue::consumeEvent()
{
    if(!this->eventQueue.empty()){
        
        const BLE::Event eventInLine = this->eventQueue.front();
        this->eventQueue.pop();
        return eventInLine;

    }
    return BLE::Event::BT_NO_EVENT;
    
}
