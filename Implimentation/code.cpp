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
https://github.com/tejasvsinghnagarkoti-234/Gridrescue/tree/main
#endif

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

#ifndef CONSUMER_H
#define CONSUMER_H

#include <string>
using namespace std;

class Consumer
{
public:
    int id;
    string name;
    int requiredPower;
    int priority;
Consumer(int id, string name, int requiredPower, int priority);
};

#endif

#ifndef TRANSMISSIONLINE_H
#define TRANSMISSIONLINE_H

class TransmissionLine
{
public:
    int from;
    int to;
    int distance;
    bool active;

    TransmissionLine(int from, int to, int distance);
};

#endif
