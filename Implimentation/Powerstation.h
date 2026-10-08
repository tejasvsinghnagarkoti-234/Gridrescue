#include "PowerStation.h"

PowerStation::PowerStation(int id, string name, int capacity)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->availablePower = capacity;
}
