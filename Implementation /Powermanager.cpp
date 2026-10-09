#include <iostream>
#include <queue>

#include "PowerManager.h"

using namespace std;

void PowerManager::redistribute(
    vector<Substation> &substations,
    vector<Consumer> &consumers)
{
    // Priority queue for consumers
    priority_queue<
        pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>
    > pq;

    for(int i = 0; i < consumers.size(); i++)
    {
        pq.push({
            consumers[i].priority,
            i
        });
    }

    // Reset loads
    for(int i = 0; i < substations.size(); i++)
    {
        if(substations[i].failed == false)
        {
            substations[i].currentLoad = 0;
        }
    }

    cout << "\n------- POWER REDISTRIBUTION -------\n";

    while(!pq.empty())
    {
        int consumerIndex = pq.top().second;

        pq.pop();

        int best = -1;

        int lowestLoad = 100000;

        // Find least loaded substation
        for(int i = 0; i < substations.size(); i++)
        {
            if(substations[i].failed == false)
            {
                if(substations[i].currentLoad < lowestLoad &&
                   substations[i].currentLoad <
                   substations[i].capacity)
                {
                    lowestLoad =
                        substations[i].currentLoad;

                    best = i;
                }
            }
        }

        if(best == -1)
        {
            cout << consumers[consumerIndex].name
                 << " cannot get power.\n";

            continue;
        }

        int available =
            substations[best].capacity -
            substations[best].currentLoad;

        int power =
            consumers[consumerIndex].requiredPower;

        if(power > available)
        {
            power = available;
        }

        substations[best].currentLoad += power;

        cout << consumers[consumerIndex].name
             << " gets "
             << power
             << " kW from "
             << substations[best].name
             << endl;
    }

    cout << "------------------------------------\n";
}
