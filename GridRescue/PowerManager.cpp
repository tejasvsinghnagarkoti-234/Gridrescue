#include <iostream>
#include <queue>
#include <algorithm>

#include "PowerManager.h"

using namespace std;

RedistributionResult PowerManager::allocate(
    vector<Substation> &substations,
    vector<Consumer> &consumers,
    const vector<int> &group,
    const vector<int> &budgets)
{
    RedistributionResult result;
    result.delivered.assign(consumers.size(), 0);
    result.totalSupplied = 0;
    result.supplyBudget = 0;
    result.groupBudget = budgets;
    result.groupSupplied.assign(budgets.size(), 0);

    for(int b : budgets) result.supplyBudget += b;

    vector<int> left = budgets;   // remaining kW per island

    // Priority queue for consumers (lowest priority number first)
    priority_queue<
        pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>
    > pq;

    for(size_t i = 0; i < consumers.size(); i++)
    {
        pq.push({consumers[i].priority, (int)i});
    }

    // Reset loads
    for(size_t i = 0; i < substations.size(); i++)
    {
        substations[i].currentLoad = 0;
    }

    while(!pq.empty())
    {
        int c = pq.top().second;
        pq.pop();

        int need = consumers[c].requiredPower;

        while(need > 0)
        {
            // Least loaded usable substation that still has room and whose
            // island still has generation left.
            int best = -1;
            int lowestLoad = 1000000000;

            for(size_t i = 0; i < substations.size(); i++)
            {
                int g = i < group.size() ? group[i] : -1;

                if(g >= 0 && g < (int)left.size() && left[g] > 0 &&
                   !substations[i].failed &&
                   substations[i].currentLoad < substations[i].capacity &&
                   substations[i].currentLoad < lowestLoad)
                {
                    lowestLoad = substations[i].currentLoad;
                    best = (int)i;
                }
            }

            if(best == -1)
            {
                break;
            }

            int g = group[best];
            int give = min(need,
                           substations[best].capacity - substations[best].currentLoad);
            give = min(give, left[g]);

            substations[best].currentLoad += give;
            left[g] -= give;
            need -= give;
            result.delivered[c] += give;
            result.totalSupplied += give;
            result.groupSupplied[g] += give;
            result.allocations.push_back({c, best, give});
        }
    }

    return result;
}

RedistributionResult PowerManager::allocate(
    vector<Substation> &substations,
    vector<Consumer> &consumers,
    const vector<bool> &usable,
    int supplyBudget)
{
    vector<int> group(substations.size(), -1);
    for(size_t i = 0; i < substations.size(); i++)
    {
        bool ok = i < usable.size() ? usable[i] : !substations[i].failed;
        if(ok) group[i] = 0;
    }
    return allocate(substations, consumers, group, vector<int>{supplyBudget});
}

static void printResult(const RedistributionResult &r,
                        vector<Substation> &substations,
                        vector<Consumer> &consumers)
{
    cout << "\n------- POWER REDISTRIBUTION -------\n";

    for(size_t i = 0; i < r.allocations.size(); i++)
    {
        cout << consumers[r.allocations[i].consumerIndex].name
             << " gets " << r.allocations[i].power << " kW from "
             << substations[r.allocations[i].substationIndex].name << endl;
    }

    for(size_t i = 0; i < consumers.size(); i++)
    {
        if(r.delivered[i] < consumers[i].requiredPower)
        {
            cout << consumers[i].name
                 << (r.delivered[i] == 0 ? " cannot get power.\n"
                                         : " is only partially supplied.\n");
        }
    }

    cout << "Supplied " << r.totalSupplied << " of " << r.supplyBudget << " kW available.\n";
    cout << "------------------------------------\n";
}

void PowerManager::redistribute(
    vector<Substation> &substations,
    vector<Consumer> &consumers,
    const vector<int> &group,
    const vector<int> &budgets)
{
    printResult(allocate(substations, consumers, group, budgets), substations, consumers);
}

void PowerManager::redistribute(
    vector<Substation> &substations,
    vector<Consumer> &consumers,
    const vector<bool> &usable,
    int supplyBudget)
{
    printResult(allocate(substations, consumers, usable, supplyBudget), substations, consumers);
}
