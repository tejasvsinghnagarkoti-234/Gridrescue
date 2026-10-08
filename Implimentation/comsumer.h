#include "PowerStation.h"

PowerStation::PowerStation(int id, string name, int capacity)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->availablePower = capacity;
}

#include "Consumer.h"

Consumer::Consumer(int id, string name, int requiredPower, int priority)
{
    this->id = id;
    this->name = name;
    this->requiredPower = requiredPower;
    this->priority = priority;
}
