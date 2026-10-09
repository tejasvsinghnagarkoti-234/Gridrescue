#include <iostream>
#include "Graph.h"

using namespace std;

Graph::Graph(int n)
{
    adj.resize(n);
    names.resize(n);
}

void Graph::addNode(int id, string name)
{
    names[id] = name;
}

void Graph::addEdge(int u, int v, int weight)
{
    adj[u].push_back({v, weight});
    adj[v].push_back({u, weight});
}

void Graph::display()
{
    cout << "\n---------- POWER GRID ----------\n";

    for(int i = 0; i < adj.size(); i++)
    {
        cout << names[i] << " -> ";

        for(int j = 0; j < adj[i].size(); j++)
        {
            cout << names[adj[i][j].first]
                 << "(" << adj[i][j].second << " km) ";

        }

        cout << endl;
    }

    cout << "--------------------------------\n";
}

int Graph::size()
{
    return adj.size();
}

vector<pair<int, int>> Graph::getNeighbours(int node)
{
    return adj[node];
}

string Graph::getName(int node)
{
    return names[node];
}
