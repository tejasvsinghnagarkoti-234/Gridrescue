#ifndef POWERSTATION_H
#define POWERSTATION_H

#include <string>
using namespace std;

class PowerStation
{
public:
    int id;
    string name;
    int capacity;
    int availablePower;

    PowerStation(int id, string name, int capacity);
};

#endif
