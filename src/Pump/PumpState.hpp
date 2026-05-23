#pragma once

#include "Pump/PumpEvents.hpp"

class PumpState{
    private:

    public:
        enum class State{
            ST_PUMP_IDLE,
            ST_PUMP_ACTIVE,
            ST_PUMP_ERROR
        };
        void handleEvent(PUMP::Event event);


};