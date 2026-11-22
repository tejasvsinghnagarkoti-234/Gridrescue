#ifndef SUBSTATION_H
#define SUBSTATION_H

#include <string>
using namespace std;

class Substation
{
public:
    int id;
    string name;
    int capacity;
    int currentLoad;
    bool failed;
    int x;   // position on the web map
    int y;

    Substation(int id, string name, int capacity, int x = 0, int y = 0);
};

#endif
