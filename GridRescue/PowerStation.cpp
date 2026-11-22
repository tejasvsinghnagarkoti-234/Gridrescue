#include "PowerStation.h"

PowerStation::PowerStation(int id, string name, int capacity, int x, int y)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->availablePower = capacity;
    this->failed = false;
    this->x = x;
    this->y = y;
}
