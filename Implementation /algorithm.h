#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <vector>
#include "Graph.h"

using namespace std;

vector<int> bfs(Graph &g, int start, vector<bool> failed);

vector<int> dfs(Graph &g, int start, vector<bool> failed);

vector<int> dijkstra(Graph &g, int start, int end, vector<bool> failed);

#endif
