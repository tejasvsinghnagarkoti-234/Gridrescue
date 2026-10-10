#ifndef GRIDMANAGER_H
#define GRIDMANAGER_H

#include <vector>
#include <unordered_map>

#include "PowerStation.h"
#include "Substation.h"
#include "Consumer.h"
#include "TransmissionLine.h"
#include "Graph.h"
#include "FailureManager.h"
#include "PowerManager.h"

using namespace std;

class GridManager
{
private:
    vector<PowerStation> stations;
    vector<Substation> substations;
    vector<Consumer> consumers;
    vector<TransmissionLine> lines;

    Graph graph;

    FailureManager failureManager;
    PowerManager powerManager;

    unordered_map<string, int> components;

public:
    GridManager();

    void createGrid();

    void displayGrid();

    void createFailure(int id);

    void undoFailure();

    void affectedArea();

    void checkConnection(int start, int end);

    void alternatePath(int start, int end);

    void redistributePower();

    void searchComponent(string name);

    void showSubstations();

    void showConsumers();

    void showFailures();
};

#endif
