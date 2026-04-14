/*
    This BaseStateMachine is not in use...Find a way to move common logic from all sub FSMs to this Base state 
*/

class BaseState{
    protected:
        enum class State;
        enum class Event;
        virtual void handleEvent(const Event event) = 0; //pure abstract fuction hyt3ml override fel child class
};