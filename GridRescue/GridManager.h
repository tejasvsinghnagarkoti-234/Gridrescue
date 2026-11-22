#ifndef GRIDMANAGER_H
#define GRIDMANAGER_H

#include <vector>
#include <unordered_map>
#include <string>

#include "PowerStation.h"
#include "Substation.h"
#include "Consumer.h"
#include "TransmissionLine.h"
#include "Graph.h"
#include "FailureManager.h"
#include "PowerManager.h"

using namespace std;

// Node IDs are shared by power stations and substations and double as graph
// vertex indices. When a node is deleted its ID becomes free and is reused by
// the next node that is added (lowest free ID first).
class GridManager
{
public:
    static const int MAP_W = 1000;        // web map coordinate space
    static const int MAP_H = 560;
    static const int MAX_NODES = 60;
    static const int MAX_CONSUMERS = 40;

private:
    vector<PowerStation> stations;
    vector<Substation> substations;
    vector<Consumer> consumers;
    vector<TransmissionLine> lines;

    Graph graph;

    FailureManager failureManager;
    PowerManager powerManager;

    unordered_map<string, int> components;
    int nextConsumerId = 1;

    // ---- helpers
    int stationIndex(int id) const;
    int substationIndex(int id) const;
    int lineIndex(int a, int b) const;
    bool isNode(int id) const;
    bool isFailed(int id) const;
    void setFailed(int id, bool value);
    string nodeName(int id) const;
    int graphSize() const;
    int freeId() const;
    bool nameTaken(const string &name, int exceptId) const;
    string autoName(const string &prefix) const;

    // Rebuilds the graph, line states and the name index from the component
    // lists. Called after every structural change.
    void rebuild(bool resetLoads = true);

    vector<bool> failedFlags();          // indexed by node ID; unused IDs = true
    vector<int> sourceIds();             // working power stations
    vector<int> reachable();             // multi-source BFS from all sources
    vector<bool> usableSubstations();    // indexed by substation vector index

    // Splits the powered grid into islands (connected components that contain
    // at least one working power station).
    void islands(vector<int> &group, vector<int> &budgets, vector<vector<int>> &islandStations);

public:
    GridManager();

    // Creates the preset grid with N substations (default 4) and one PS1.
    void createGrid(int numSubstations = 4);

    // Removes every power station, substation and line (consumers stay).
    void clearGrid();

    void displayGrid();

    void createFailure(int id);

    void undoFailure();

    void restoreAll();

    void affectedArea();

    void checkConnection(int start, int end);

    void alternatePath(int start, int end);

    void redistributePower();

    void searchComponent(string name);

    void showSubstations();

    void showConsumers();

    void showFailures();

    // Dynamic line addition
    bool addTransmissionLine(int from, int to, int distance);

    // ---- Interactive editing. Each returns "" on success or an error message.
    string addNode(bool isStation, string name, int capacity, int x, int y, int &newId);
    string moveNode(int id, int x, int y);
    string updateNode(int id, const string &name, int capacity);
    string removeNode(int id);
    string updateLine(int from, int to, int distance);
    string removeLine(int from, int to);
    string addConsumer(const string &name, int required, int priority, int &newId);
    string removeConsumer(int id);

    int getNumSubstations() const { return (int)substations.size(); }
    int getNumStations() const { return (int)stations.size(); }
    int getTotalNodes() const { return (int)(substations.size() + stations.size()); }

    // ---- Web/JSON API (see GridJson.cpp)
    string apiState();
    string apiFail(int id);
    string apiUndo();
    string apiRestore();
    string apiAffected();
    string apiConnection(int start, int end);
    string apiPath(int start, int end);
    string apiRedistribute();
    string apiSearch(const string &name);
    string apiAddLine(int from, int to, int distance);
    string apiAddNode(const string &type, const string &name, int capacity, int x, int y);
    string apiMoveNode(int id, int x, int y);
    string apiUpdateNode(int id, const string &name, int capacity);
    string apiRemoveNode(int id);
    string apiUpdateLine(int from, int to, int distance);
    string apiRemoveLine(int from, int to);
    string apiClear();
    string apiAddConsumer(const string &name, int required, int priority);
    string apiRemoveConsumer(int id);
};

#endif
