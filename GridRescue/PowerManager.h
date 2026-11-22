#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <vector>
#include "Substation.h"
#include "Consumer.h"

using namespace std;

// One chunk of power delivered to a consumer by a substation.
struct Allocation
{
    int consumerIndex;
    int substationIndex;
    int power;
};

struct RedistributionResult
{
    vector<Allocation> allocations;
    vector<int> delivered;      // kW delivered per consumer
    int totalSupplied;          // kW drawn from all power stations
    int supplyBudget;           // kW all reachable power stations can produce
    vector<int> groupBudget;    // per island: kW its power stations can produce
    vector<int> groupSupplied;  // per island: kW actually drawn
};

class PowerManager
{
public:
    // Pure computation: serves consumers in priority order (1 = most important),
    // always picking the least loaded usable substation. Updates currentLoad.
    //
    // group[i]   = island (connected component) substation i belongs to, or -1
    //              if no working power station can reach it.
    // budgets[k] = kW the power stations inside island k can produce.
    // A substation can only hand out power while its own island has budget left,
    // so a power station never "feeds" a part of the grid it is not wired to.
    RedistributionResult allocate(
        vector<Substation> &substations,
        vector<Consumer> &consumers,
        const vector<int> &group,
        const vector<int> &budgets
    );

    // Single-island version (one power station), kept for compatibility.
    RedistributionResult allocate(
        vector<Substation> &substations,
        vector<Consumer> &consumers,
        const vector<bool> &usable,
        int supplyBudget
    );

    // Console versions (print the result).
    void redistribute(
        vector<Substation> &substations,
        vector<Consumer> &consumers,
        const vector<int> &group,
        const vector<int> &budgets
    );

    void redistribute(
        vector<Substation> &substations,
        vector<Consumer> &consumers,
        const vector<bool> &usable,
        int supplyBudget
    );
};
#endif
