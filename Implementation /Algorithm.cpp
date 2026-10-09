#include <iostream>
#include <queue>
#include <stack>
#include <climits>

#include "Algorithms.h"

using namespace std;


vector<int> bfs(Graph &g, int start, vector<bool> failed)
{
    vector<bool> visited(g.size(), false);
    vector<int> result;

    queue<int> q;

    if(failed[start])
    {
        return result;
    }

    q.push(start);
    visited[start] = true;

    while(!q.empty())
    {
        int current = q.front();
        q.pop();

        result.push_back(current);

        vector<pair<int, int>> neighbours =  g.getNeighbours(current);
           

        for(int i = 0; i < neighbours.size(); i++)
        {
            int next = neighbours[i].first;

            if(visited[next] == false &&
               failed[next] == false)
            {
                visited[next] = true;
                q.push(next);
            }
        }
    }

    return result;
}


vector<int> dfs(Graph &g, int start, vector<bool> failed)
{
    vector<bool> visited(g.size(), false);
    vector<int> result;

    stack<int> s;

    if(failed[start])
    {
        return result;
    }

    s.push(start);

    while(!s.empty())
    {
        int current = s.top();
        s.pop();

        if(visited[current] == true || failed[current] == true)
           
        {
            continue;
        }

        visited[current] = true;
        result.push_back(current);

        vector<pair<int, int>> neighbours = g.getNeighbours(current);

            
        for(int i = 0; i < neighbours.size(); i++)
        {
            int next = neighbours[i].first;

            if(visited[next] == false && failed[next] == false)
               
            {
                s.push(next);
            }
        }
    }

    return result;
}

vector<int> dijkstra(
    Graph &g,
    int start,
    int end,
    vector<bool> failed)
{
    int n = g.size();

    vector<int> distance(n, INT_MAX);
    vector<int> parent(n, -1);

    priority_queue<
        pair<int, int>,
        vector<pair<int, int>>,
        greater<pair<int, int>>
    > pq;

    if(failed[start] || failed[end])
    {
        return {};
    }

    distance[start] = 0;

    pq.push({0, start});

    while(!pq.empty())
    {
        int current = pq.top().second;
        int currentDistance = pq.top().first;

        pq.pop();

        if(currentDistance > distance[current])
        {
            continue;
        }

        vector<pair<int, int>> neighbours = g.getNeighbours(current);
            

         for(int i = 0; i < neighbours.size(); i++)
        {
            int next = neighbours[i].first;
            int weight = neighbours[i].second;

            if(failed[next])
            {
                continue;
            }

            if(distance[current] + weight <  distance[next])
               
            {
                distance[next] = distance[current] + weight;

                parent[next] = current;

                pq.push({
                    distance[next],
                    next
                });
            }
        }
    }

    if(distance[end] == INT_MAX)
    {
        return {};
    }

    vector<int> path;

    int current = end;

    while(current != -1)
    {
        path.push_back(current);
        current = parent[current];
    }

    
    for(int i = 0; i < path.size() / 2; i++)
    {
        int temp = path[i];

        path[i] =
            path[path.size() - 1 - i];

        path[path.size() - 1 - i] =
            temp;
    }

    return path;
}
