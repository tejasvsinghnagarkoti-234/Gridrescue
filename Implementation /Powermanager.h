#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <vector>
#include "Substation.h"
#include "Consumer.h"

using namespace std;

class PowerManager
{
public:
    void redistribute(
        vector<Substation> &substations,
        vector<Consumer> &consumers
    );
};
#endif
