#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <vector>
#include "Graph.h"

using namespace std;

vector<int> bfs(Graph &g, int start, vector<bool> failed);

// Multi-source BFS: every start node is enqueued at level 0.
// Used when the grid has more than one power station.
vector<int> bfsMulti(Graph &g, const vector<int> &starts, vector<bool> failed);

vector<int> dfs(Graph &g, int start, vector<bool> failed);

vector<int> dijkstra(Graph &g, int start, int end, vector<bool> failed);

#endif
