#include <iostream>

#include "GridManager.h"
#include "Algorithms.h"

using namespace std;

GridManager::GridManager()
    : graph(5)
{
}

void GridManager::createGrid()
{
    
    stations.push_back(
        PowerStation(0, "PS1", 1000)
    );

    
    substations.push_back(
        Substation(1, "SS1", 500)
    );

    substations.push_back(
        Substation(2, "SS2", 500)
    );

    substations.push_back(
        Substation(3, "SS3", 500)
    );

    substations.push_back(
        Substation(4, "SS4", 500)
    );

   
    consumers.push_back(
        Consumer(1, "Hospital", 200, 1)
    );

    consumers.push_back(
        Consumer(2, "EmergencyCenter", 150, 2)
    );

    consumers.push_back(
        Consumer(3, "School", 100, 3)
    );

    consumers.push_back(
        Consumer(4, "ResidentialArea", 250, 4)
    );

  
    lines.push_back(
        TransmissionLine(0, 1, 10)
    );

    lines.push_back(
        TransmissionLine(1, 2, 15)
    );

    lines.push_back(
        TransmissionLine(2, 3, 20)
    );

    lines.push_back(
        TransmissionLine(1, 4, 25)
    );

    lines.push_back(
        TransmissionLine(4, 3, 10)
    );

   
    graph.addNode(0, "PS1");

    for(int i = 0; i < substations.size(); i++)
    {
        graph.addNode(
            substations[i].id,
            substations[i].name
        );
    }

   
    for(int i = 0; i < lines.size(); i++)
    {
        if(lines[i].active)
        {
            graph.addEdge(
                lines[i].from,
                lines[i].to,
                lines[i].distance
            );
        }
    }

   
    for(int i = 0; i < stations.size(); i++)
    {
        components[stations[i].name] =
            stations[i].id;
    }

    for(int i = 0; i < substations.size(); i++)
    {
        components[substations[i].name] =
            substations[i].id;
    }
}

void GridManager::displayGrid()
{
    graph.display();
}

void GridManager::createFailure(int id)
{
    if(id < 1 || id > 4)
    {
        cout << "Invalid substation ID.\n";
        return;
    }

    int index = id - 1;

    if(substations[index].failed)
    {
        cout << "Substation already failed.\n";
        return;
    }

    substations[index].failed = true;

    failureManager.addFailure(id);

   
    for(int i = 0; i < lines.size(); i++)
    {
        if(lines[i].from == id ||
           lines[i].to == id)
        {
            lines[i].active = false;
        }
    }

   
    graph = Graph(5);

    graph.addNode(0, "PS1");

    for(int i = 0; i < substations.size(); i++)
    {
        graph.addNode(
            substations[i].id,
            substations[i].name
        );
    }

    for(int i = 0; i < lines.size(); i++)
    {
        if(lines[i].active)
        {
            graph.addEdge(
                lines[i].from,
                lines[i].to,
                lines[i].distance
            );
        }
    }

    cout << substations[index].name
         << " has failed.\n";
}

void GridManager::undoFailure()
{
    int id = failureManager.undoFailure();

    if(id == -1)
    {
        cout << "No failure to undo.\n";
        return;
    }

    int index = id - 1;

    substations[index].failed = false;

   
    for(int i = 0; i < lines.size(); i++)
    {
        if(lines[i].from == id ||
           lines[i].to == id)
        {
            lines[i].active = true;
        }
    }

    for(int i = 0; i < lines.size(); i++)
    {
        int a = lines[i].from;
        int b = lines[i].to;

        if(a >= 1 && a <= 4)
        {
            if(substations[a - 1].failed)
            {
                lines[i].active = false;
            }
        }

        if(b >= 1 && b <= 4)
        {
            if(substations[b - 1].failed)
            {
                lines[i].active = false;
            }
        }
    }

  
    graph = Graph(5);

    graph.addNode(0, "PS1");

    for(int i = 0; i < substations.size(); i++)
    {
        graph.addNode(
            substations[i].id,
            substations[i].name
        );
    }

    for(int i = 0; i < lines.size(); i++)
    {
        if(lines[i].active)
        {
            graph.addEdge(
                lines[i].from,
                lines[i].to,
                lines[i].distance
            );
        }
    }

    cout << substations[index].name
         << " failure has been undone.\n";
}

void GridManager::affectedArea()
{
    vector<bool> failed(5, false);

    for(int i = 0; i < substations.size(); i++)
    {
        failed[substations[i].id] =
            substations[i].failed;
    }

    vector<int> connected =
        bfs(graph, 0, failed);

    cout << "\n------- AFFECTED AREAS -------\n";

    for(int i = 1; i <= 4; i++)
    {
        bool found = false;

        for(int j = 0; j < connected.size(); j++)
        {
            if(connected[j] == i)
            {
                found = true;
            }
        }

        cout << graph.getName(i) << " : ";

        if(found)
        {
            cout << "Connected\n";
        }
        else
        {
            cout << "Affected\n";
        }
    }

    cout << "------------------------------\n";
}

void GridManager::checkConnection(int start, int end)
{
    vector<bool> failed(5, false);

    for(int i = 0; i < substations.size(); i++)
    {
        failed[substations[i].id] =
            substations[i].failed;
    }

    vector<int> result =
        dfs(graph, start, failed);

    bool found = false;

    for(int i = 0; i < result.size(); i++)
    {
        if(result[i] == end)
        {
            found = true;
        }
    }

    cout << "\n";

    if(found)
    {
        cout << "Connection exists.\n";
    }
    else
    {
        cout << "No connection exists.\n";
    }
}

void GridManager::alternatePath(int start, int end)
{
    vector<bool> failed(5, false);

    for(int i = 0; i < substations.size(); i++)
    {
        failed[substations[i].id] =
            substations[i].failed;
    }

    vector<int> path =
        dijkstra(
            graph,
            start,
            end,
            failed
        );

    cout << "\n------- BEST PATH -------\n";

    if(path.empty())
    {
        cout << "No path available.\n";
        return;
    }

    for(int i = 0; i < path.size(); i++)
    {
        cout << graph.getName(path[i]);

        if(i != path.size() - 1)
        {
            cout << " -> ";
        }
    }

    cout << "\n-------------------------\n";
}

void GridManager::redistributePower()
{
    powerManager.redistribute(
        substations,
        consumers
    );
}

void GridManager::searchComponent(string name)
{
    if(components.find(name) == components.end())
    {
        cout << "Component not found.\n";
        return;
    }

    cout << "Component found.\n";
    cout << "Name: " << name << endl;
    cout << "ID: "
         << components[name]
         << endl;
}

void GridManager::showSubstations()
{
    cout << "\n------- SUBSTATIONS -------\n";

    for(int i = 0; i < substations.size(); i++)
    {
        cout << "ID: "
             << substations[i].id
             << " | "
             << substations[i].name
             << " | Load: "
             << substations[i].currentLoad
             << " kW"
             << " | ";

        if(substations[i].failed)
        {
            cout << "FAILED";
        }
        else
        {
            cout << "ACTIVE";
        }

        cout << endl;
    }

    cout << "---------------------------\n";
}

void GridManager::showConsumers()
{
    cout << "\n------- CONSUMERS -------\n";

    for(int i = 0; i < consumers.size(); i++)
    {
        cout << consumers[i].id
             << " | "
             << consumers[i].name
             << " | "
             << consumers[i].requiredPower
             << " kW"
             << " | Priority: "
             << consumers[i].priority
             << endl;
    }

    cout << "-------------------------\n";
}

void GridManager::showFailures()
{
    failureManager.display();
}
