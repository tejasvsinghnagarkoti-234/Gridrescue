#include "Substation.h"

Substation::Substation(int id, string name, int capacity)
{
    this->id = id;
    this->name = name;
    this->capacity = capacity;
    this->currentLoad = 0;
    this->failed = false;
}
