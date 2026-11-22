#include "Substation.h"

Substation::Substation(int id, string name, int capacity, int x, int y)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->currentLoad = 0;
    this->failed = false;
    this->x = x;
    this->y = y;
}
