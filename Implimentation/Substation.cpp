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

    Substation(int id, string name, int capacity);
};

#endif
