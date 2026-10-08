#include "PowerStation.h"

PowerStation::PowerStation(int id, string name, int capacity)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->availablePower = capacity;
}

#include "Substation.h"

Substation::Substation(int id, string name, int capacity)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->currentLoad = 0;
    this->failed = false;
}

#include "Consumer.h"

Consumer::Consumer(int id, string name, int requiredPower, int priority)
{
    this->id = id;
    this->name = name;
    this->requiredPower = requiredPower;
    this->priority = priority;
}

#include "TransmissionLine.h"

TransmissionLine::TransmissionLine(int from, int to, int distance)
{
    this->from = from;
    this->to = to;
    this->distance = distance;
    this->active = true;
}
