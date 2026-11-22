#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>

using namespace std;

class Graph
{
private:
    vector<vector<pair<int, int>>> adj;
    vector<string> names;

public:
    Graph(int n);

    void addNode(int id, string name);
    void addEdge(int u, int v, int weight);

    void display();

    int size();

    vector<pair<int, int>> getNeighbours(int node);
    string getName(int node);
};

#endif
