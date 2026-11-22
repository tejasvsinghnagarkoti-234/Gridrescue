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
    bool failed;   // a power station can trip too (it then supplies nothing)
    int x;         // position on the web map (map units, 0..1000 x 0..560)
    int y;

    PowerStation(int id, string name, int capacity, int x = 0, int y = 0);
};

#endif
